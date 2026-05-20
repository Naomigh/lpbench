#include "bench_common.hpp"
#include "dataset.hpp"
#include "leap_raw_warm_kernel_interface.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Args {
    std::string dataset;
    std::string out_dir = "benchmark/results";
    std::vector<std::string> backends = {"leap_avx2", "leap_avx512"};
    std::vector<int> ks = {1, 2, 3, 4, 5};
    int length = bench::kDefaultLength;
    int true_ed_min = bench::kDefaultTrueEdMin;
    int true_ed_max = bench::kDefaultTrueEdMax;
    int repeat = 64;
    int seed = bench::kDefaultSeed;
    int do_warmup = 1;
};

std::vector<std::string> read_values(int& i, int argc, char** argv, const char* name) {
    std::vector<std::string> values;
    while (i + 1 < argc && !bench::starts_with_dash(argv[i + 1])) {
        values.emplace_back(argv[++i]);
    }
    if (values.empty()) {
        throw std::runtime_error(std::string("missing value for ") + name);
    }
    return values;
}

Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        auto one = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++i];
        };
        if (key == "--dataset") {
            args.dataset = one("--dataset");
        } else if (key == "--out-dir") {
            args.out_dir = one("--out-dir");
        } else if (key == "--backends") {
            args.backends = read_values(i, argc, argv, "--backends");
        } else if (key == "--backend") {
            args.backends = {one("--backend")};
        } else if (key == "--ks") {
            args.ks.clear();
            for (const auto& v : read_values(i, argc, argv, "--ks")) {
                args.ks.push_back(bench::parse_int(v, "--ks"));
            }
        } else if (key == "--k") {
            args.ks = {bench::parse_int(one("--k"), "--k")};
        } else if (key == "--length") {
            args.length = bench::parse_int(one("--length"), "--length");
        } else if (key == "--true-ed-min") {
            args.true_ed_min = bench::parse_int(one("--true-ed-min"), "--true-ed-min");
        } else if (key == "--true-ed-max") {
            args.true_ed_max = bench::parse_int(one("--true-ed-max"), "--true-ed-max");
        } else if (key == "--repeat") {
            args.repeat = bench::parse_int(one("--repeat"), "--repeat");
        } else if (key == "--seed") {
            args.seed = bench::parse_int(one("--seed"), "--seed");
        } else if (key == "--no-warmup") {
            args.do_warmup = 0;
        } else {
            throw std::runtime_error("unknown argument: " + key);
        }
    }
    if (args.dataset.empty()) {
        args.dataset = bench::default_dataset_path(args.length, args.true_ed_min, args.true_ed_max);
    }
    if (args.repeat <= 0) {
        throw std::runtime_error("--repeat must be positive");
    }
    return args;
}

int chunk_size_for_backend(const std::string& backend) {
    if (backend == "leap_avx2") {
        return 256;
    }
    if (backend == "leap_avx512") {
        return 512;
    }
    throw std::runtime_error("unknown LEAP backend: " + backend);
}

BenchLeapRawWarmResult run_chunk(
    const std::string& backend, const char* read, const char* ref, int length, int k, int repeat, int do_warmup) {
    if (backend == "leap_avx2") {
        return bench_leap_avx2_raw_warm_kernel_once(read, ref, length, k, repeat, do_warmup);
    }
    if (backend == "leap_avx512") {
        return bench_leap_avx512_raw_warm_kernel_once(read, ref, length, k, repeat, do_warmup);
    }
    throw std::runtime_error("unknown LEAP backend: " + backend);
}

struct RawWarmTotals {
    std::size_t pair_count = 0;
    uint64_t chunk_count = 0;
    uint64_t total_iterations = 0;
    uint64_t total_cpu_ns = 0;
    uint64_t total_cycles = 0;
    uint64_t pass_count = 0;
    uint64_t fail_count = 0;
    uint64_t mismatch_count = 0;
};

RawWarmTotals measure_raw_warm(
    const std::string& backend,
    const std::vector<const bench::PairRecord*>& pairs,
    int k,
    int chunk_size,
    int repeat,
    int do_warmup) {
    RawWarmTotals totals;
    totals.pair_count = pairs.size();

    for (const auto* p : pairs) {
        bool pair_pass = true;
        for (const auto& [offset, n] : bench::chunks_for_length(p->length, chunk_size)) {
            auto result = run_chunk(
                backend, p->read.data() + offset, p->reference.data() + offset, n, k, repeat, do_warmup);
            pair_pass = pair_pass && (result.pass != 0);
            totals.chunk_count += static_cast<uint64_t>(result.chunk_count);
            totals.total_iterations += static_cast<uint64_t>(repeat);
            totals.total_cpu_ns += result.total_cpu_ns;
            totals.total_cycles += result.total_cycles;
        }

        if (pair_pass) {
            ++totals.pass_count;
        } else {
            ++totals.fail_count;
        }
        if (static_cast<uint8_t>(pair_pass ? 1 : 0) != bench::expected_pass_for_k(*p, k)) {
            ++totals.mismatch_count;
        }
    }
    return totals;
}

double div0(uint64_t num, uint64_t den) {
    return den == 0 ? 0.0 : static_cast<double>(num) / static_cast<double>(den);
}

void write_row(std::ofstream& out,
               const std::string& backend,
               const Args& args,
               int k,
               int ed,
               int chunk_size,
               const RawWarmTotals& t) {
    out << backend << '\t'
        << "raw_warm" << '\t'
        << args.length << '\t'
        << k << '\t'
        << ed << '\t'
        << t.pair_count << '\t'
        << chunk_size << '\t'
        << std::fixed << std::setprecision(6)
        << (t.pair_count == 0 ? 0.0 : static_cast<double>(t.chunk_count) / static_cast<double>(t.pair_count)) << '\t'
        << args.repeat << '\t'
        << t.total_iterations << '\t'
        << t.total_cpu_ns << '\t'
        << div0(t.total_cpu_ns, t.total_iterations) << '\t'
        << div0(t.total_cpu_ns, static_cast<uint64_t>(t.pair_count)) << '\t'
        << t.total_cycles << '\t'
        << div0(t.total_cycles, t.total_iterations) << '\t'
        << div0(t.total_cycles, static_cast<uint64_t>(t.pair_count)) << '\t'
        << t.pass_count << '\t'
        << t.fail_count << '\t'
        << t.mismatch_count << '\t'
        << bench_leap_raw_touch_sink << '\t'
        << args.seed << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Args args = parse_args(argc, argv);
        auto pairs = bench::load_dataset_tsv(args.dataset);

        std::filesystem::path raw_dir = std::filesystem::path(args.out_dir) / "raw";
        std::filesystem::create_directories(raw_dir);
        std::filesystem::path out_path = raw_dir / "leap_raw_warm_kernel_raw.tsv";
        std::ofstream out(out_path);
        if (!out) {
            throw std::runtime_error("failed to open " + out_path.string());
        }

        out << "backend\tkernel_mode\tlength\tk\ttrue_ed_group\tpair_count\tchunk_size\t"
            << "avg_chunk_count_per_pair\trepeat\ttotal_iterations\ttotal_cpu_ns\t"
            << "avg_cpu_ns_per_iteration\tavg_cpu_ns_per_pair\ttotal_cycles\t"
            << "avg_cycles_per_iteration\tavg_cycles_per_pair\tpass_count\tfail_count\t"
            << "mismatch_count\ttouch_sink\tseed\n";

        for (const auto& backend : args.backends) {
            int chunk_size = chunk_size_for_backend(backend);
            for (int k : args.ks) {
                for (int ed = args.true_ed_min; ed <= args.true_ed_max; ++ed) {
                    auto selected = bench::select_pairs_by_ed(pairs, args.length, ed);
                    auto totals = measure_raw_warm(backend, selected, k, chunk_size, args.repeat, args.do_warmup);
                    write_row(out, backend, args, k, ed, chunk_size, totals);
                }
            }
        }

        std::cerr << "Wrote " << out_path << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "leap_raw_warm_kernel_bench: " << e.what() << "\n";
        return 1;
    }
}

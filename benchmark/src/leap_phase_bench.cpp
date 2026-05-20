#include "bench_common.hpp"
#include "cpu_memory_path.hpp"
#include "dataset.hpp"
#include "leap_phase_interface.hpp"
#include "perf_schema.hpp"
#include "timing.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

static volatile uint64_t g_phase_sink = 0;

struct Args {
    std::string dataset;
    std::string out_dir = "benchmark/results";
    std::vector<std::string> backends = {"leap_avx2", "leap_avx512"};
    std::vector<int> ks = {1, 2, 3, 4, 5};
    int length = bench::kDefaultLength;
    int true_ed_min = bench::kDefaultTrueEdMin;
    int true_ed_max = bench::kDefaultTrueEdMax;
    int seed = bench::kDefaultSeed;
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
        } else if (key == "--seed") {
            args.seed = bench::parse_int(one("--seed"), "--seed");
        } else {
            throw std::runtime_error("unknown argument: " + key);
        }
    }
    if (args.dataset.empty()) {
        args.dataset = bench::default_dataset_path(args.length, args.true_ed_min, args.true_ed_max);
    }
    return args;
}

struct PhaseTotals {
    uint64_t construct_init = 0;
    uint64_t load_reads = 0;
    uint64_t calculate_masks = 0;
    uint64_t reset = 0;
    uint64_t run = 0;
    uint64_t check_pass = 0;
    uint64_t sink = 0;
};

int chunk_size_for_backend(const std::string& backend) {
    if (backend == "leap_avx2") {
        return 256;
    }
    if (backend == "leap_avx512") {
        return 512;
    }
    throw std::runtime_error("unknown LEAP backend: " + backend);
}

PhaseTotals measure_phases(
    const std::string& backend,
    const std::vector<const bench::PairRecord*>& pairs,
    int k,
    int chunk_size) {
    PhaseTotals t;

    for (const auto* p : pairs) {
        for (const auto& [offset, n] : bench::chunks_for_length(p->length, chunk_size)) {
            BenchLeapPhaseResult phase;
            if (backend == "leap_avx2") {
                phase = bench_leap_avx2_phase_once(p->read.data() + offset, p->reference.data() + offset, n, k);
            } else if (backend == "leap_avx512") {
                phase = bench_leap_avx512_phase_once(p->read.data() + offset, p->reference.data() + offset, n, k);
            } else {
                throw std::runtime_error("unknown LEAP backend: " + backend);
            }

            t.construct_init += phase.construct_init_cpu_ns;
            t.load_reads += phase.load_reads_cpu_ns;
            t.calculate_masks += phase.calculate_masks_cpu_ns;
            t.reset += phase.reset_cpu_ns;
            t.run += phase.run_cpu_ns;
            t.check_pass += phase.check_pass_cpu_ns;

            uint64_t s = bench::thread_cpu_time_ns();
            g_phase_sink += phase.pass ? 1 : 0;
            uint64_t e = bench::thread_cpu_time_ns();
            t.sink += e - s;
        }
    }
    return t;
}

double pct(uint64_t part, uint64_t total) {
    return total == 0 ? 0.0 : (static_cast<double>(part) * 100.0 / static_cast<double>(total));
}

void write_row(std::ofstream& out,
               const std::string& backend,
               const Args& args,
               int k,
               int ed,
               std::size_t pair_count,
               int chunk_size,
               const PhaseTotals& t) {
    int chunk_count = static_cast<int>(bench::chunks_for_length(args.length, chunk_size).size());
    uint64_t total = t.construct_init + t.load_reads + t.calculate_masks + t.reset +
                     t.run + t.check_pass + t.sink;
    double avg_total = pair_count == 0 ? 0.0 : static_cast<double>(total) / static_cast<double>(pair_count);
    double avg_run = pair_count == 0 ? 0.0 : static_cast<double>(t.run) / static_cast<double>(pair_count);

    out << backend << '\t'
        << args.length << '\t'
        << k << '\t'
        << ed << '\t'
        << pair_count << '\t'
        << chunk_size << '\t'
        << chunk_count << '\t'
        << t.construct_init << '\t'
        << t.load_reads << '\t'
        << t.calculate_masks << '\t'
        << t.reset << '\t'
        << t.run << '\t'
        << t.check_pass << '\t'
        << t.sink << '\t'
        << total << '\t'
        << std::fixed << std::setprecision(3)
        << pct(t.construct_init, total) << '\t'
        << pct(t.load_reads, total) << '\t'
        << pct(t.calculate_masks, total) << '\t'
        << pct(t.reset, total) << '\t'
        << pct(t.run, total) << '\t'
        << pct(t.check_pass, total) << '\t'
        << pct(t.sink, total) << '\t'
        << avg_total << '\t'
        << avg_run << '\t'
        << args.seed << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Args args = parse_args(argc, argv);
        auto pairs = bench::load_dataset_tsv(args.dataset);
        std::filesystem::create_directories(std::filesystem::path(args.out_dir) / "raw");
        std::ofstream out(std::filesystem::path(args.out_dir) / "raw" / "leap_phase_raw.tsv");
        if (!out) {
            throw std::runtime_error("failed to open leap_phase_raw.tsv");
        }
        out << bench::kLeapPhaseHeader;

        for (const auto& backend : args.backends) {
            int chunk_size = chunk_size_for_backend(backend);
            for (int k : args.ks) {
                for (int ed = args.true_ed_min; ed <= args.true_ed_max; ++ed) {
                    auto selected = bench::select_pairs_by_ed(pairs, args.length, ed);
                    PhaseTotals totals = measure_phases(backend, selected, k, chunk_size);
                    write_row(out, backend, args, k, ed, selected.size(), chunk_size, totals);
                }
            }
        }

        std::cerr << "Wrote " << (std::filesystem::path(args.out_dir) / "raw" / "leap_phase_raw.tsv") << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "leap_phase_bench: " << e.what() << "\n";
        return 1;
    }
}

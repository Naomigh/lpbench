#include "bench_common.hpp"
#include "cpu_memory_path.hpp"
#include "dataset.hpp"
#include "method_interface.hpp"
#include "perf_schema.hpp"
#include "timing.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

static volatile uint64_t g_pass_sink = 0;

struct Args {
    std::string dataset;
    std::string out_dir = "benchmark/results";
    std::vector<std::string> methods = {
        "leap_avx2", "leap_avx512", "lv89", "miniwfa", "wfa2_fresh", "wfa2_reuse"};
    std::vector<int> ks = {1, 2, 3, 4, 5};
    int length = bench::kDefaultLength;
    int true_ed_min = bench::kDefaultTrueEdMin;
    int true_ed_max = bench::kDefaultTrueEdMax;
    int seed = bench::kDefaultSeed;
    std::string cache_mode = "warm";
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
        } else if (key == "--methods") {
            args.methods = read_values(i, argc, argv, "--methods");
        } else if (key == "--method") {
            args.methods = {one("--method")};
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
        } else if (key == "--cache-mode") {
            args.cache_mode = one("--cache-mode");
        } else {
            throw std::runtime_error("unknown argument: " + key);
        }
    }
    if (args.dataset.empty()) {
        args.dataset = bench::default_dataset_path(args.length, args.true_ed_min, args.true_ed_max);
    }
    if (args.cache_mode != "warm" && args.cache_mode != "cold-ish") {
        throw std::runtime_error("--cache-mode must be warm or cold-ish");
    }
    return args;
}

std::map<std::string, bench::MethodSpec> method_map() {
    std::map<std::string, bench::MethodSpec> out;
    for (auto method : bench::all_methods()) {
        out.emplace(method.method, method);
    }
    return out;
}

void prepare_cache(const std::string& cache_mode,
                   const bench::MethodSpec& method,
                   const std::vector<const bench::PairRecord*>& pairs,
                   int k,
                   std::vector<uint8_t>& eviction_buffer) {
    if (cache_mode == "warm") {
        for (const auto* p : pairs) {
            bench::touch_bytes(p->read, g_pass_sink);
            bench::touch_bytes(p->reference, g_pass_sink);
        }
        for (const auto* p : pairs) {
            bench::VerifyInput input{p->read.data(), p->reference.data(), p->length, k};
            auto r = method.verify(input);
            g_pass_sink += r.pass ? 1 : 0;
        }
    } else {
        bench::evict_coldish_cache(eviction_buffer, g_pass_sink);
    }
}

struct RunResult {
    uint64_t total_cpu_ns = 0;
    uint64_t pass_count = 0;
    uint64_t fail_count = 0;
    uint64_t mismatch_count = 0;
    uint64_t result_sink = 0;
};

RunResult run_once(const std::string& cache_mode,
                   const bench::MethodSpec& method,
                   const std::vector<const bench::PairRecord*>& pairs,
                   int k,
                   std::vector<uint8_t>& eviction_buffer) {
    if (pairs.empty()) {
        return {};
    }
    std::vector<uint8_t> measured(pairs.size(), 0);
    prepare_cache(cache_mode, method, pairs, k, eviction_buffer);

    uint64_t start_ns = bench::thread_cpu_time_ns();
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        const auto* p = pairs[i];
        bench::VerifyInput input{p->read.data(), p->reference.data(), p->length, k};
        bench::VerifyResult r = method.verify(input);
        uint8_t pass = r.pass ? 1 : 0;
        measured[i] = pass;
        g_pass_sink += pass;
    }
    uint64_t end_ns = bench::thread_cpu_time_ns();

    RunResult result;
    result.total_cpu_ns = end_ns - start_ns;
    result.result_sink = g_pass_sink;
    for (std::size_t i = 0; i < pairs.size(); ++i) {
        result.pass_count += measured[i] ? 1 : 0;
        result.fail_count += measured[i] ? 0 : 1;
        result.mismatch_count += measured[i] != bench::expected_pass_for_k(*pairs[i], k) ? 1 : 0;
    }
    return result;
}

void write_threshold_raw_row(std::ofstream& out,
                             const bench::MethodSpec& method,
                             const Args& args,
                             int k,
                             const RunResult& result,
                             std::size_t pair_count) {
    double avg = pair_count == 0 ? 0.0 : static_cast<double>(result.total_cpu_ns) / static_cast<double>(pair_count);
    double seconds_10m = avg * 10000000.0 / 1e9;
    out << method.method << '\t'
        << method.backend << '\t'
        << method.mode << '\t'
        << args.cache_mode << '\t'
        << args.length << '\t'
        << k << '\t'
        << pair_count << '\t'
        << result.pass_count << '\t'
        << result.fail_count << '\t'
        << result.mismatch_count << '\t'
        << result.total_cpu_ns << '\t'
        << std::fixed << std::setprecision(3) << avg << '\t'
        << std::fixed << std::setprecision(6) << seconds_10m << '\t'
        << result.result_sink << '\t'
        << args.seed << '\n';
}

void write_by_ed_row(std::ofstream& out,
                     const bench::MethodSpec& method,
                     const Args& args,
                     int k,
                     int ed,
                     const RunResult& result,
                     std::size_t pair_count) {
    double avg = pair_count == 0 ? 0.0 : static_cast<double>(result.total_cpu_ns) / static_cast<double>(pair_count);
    out << method.method << '\t'
        << method.backend << '\t'
        << method.mode << '\t'
        << args.cache_mode << '\t'
        << args.length << '\t'
        << k << '\t'
        << ed << '\t'
        << pair_count << '\t'
        << result.pass_count << '\t'
        << result.fail_count << '\t'
        << result.mismatch_count << '\t'
        << result.total_cpu_ns << '\t'
        << std::fixed << std::setprecision(3) << avg << '\t'
        << args.seed << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Args args = parse_args(argc, argv);
        auto pairs = bench::load_dataset_tsv(args.dataset);
        auto selected = bench::select_pairs(pairs, args.length, args.true_ed_min, args.true_ed_max);
        if (selected.empty()) {
            throw std::runtime_error("no dataset rows selected");
        }

        std::filesystem::create_directories(std::filesystem::path(args.out_dir) / "raw");
        std::ofstream threshold_raw(std::filesystem::path(args.out_dir) / "raw" / "threshold_raw.tsv");
        std::ofstream threshold_by_ed(std::filesystem::path(args.out_dir) / "raw" / "threshold_by_ed.tsv");
        if (!threshold_raw || !threshold_by_ed) {
            throw std::runtime_error("failed to open threshold output files");
        }
        threshold_raw << bench::kThresholdRawHeader;
        threshold_by_ed << bench::kThresholdByEdHeader;

        auto methods = method_map();
        std::vector<uint8_t> eviction_buffer(256ull * 1024ull * 1024ull, 0);

        for (const auto& method_name : args.methods) {
            auto it = methods.find(method_name);
            if (it == methods.end()) {
                throw std::runtime_error("unknown method: " + method_name);
            }
            const auto& method = it->second;
            for (int k : args.ks) {
                auto result = run_once(args.cache_mode, method, selected, k, eviction_buffer);
                write_threshold_raw_row(threshold_raw, method, args, k, result, selected.size());

                for (int ed = args.true_ed_min; ed <= args.true_ed_max; ++ed) {
                    auto by_ed = bench::select_pairs_by_ed(pairs, args.length, ed);
                    auto ed_result = run_once(args.cache_mode, method, by_ed, k, eviction_buffer);
                    write_by_ed_row(threshold_by_ed, method, args, k, ed, ed_result, by_ed.size());
                }
            }
        }

        std::cerr << "Wrote " << (std::filesystem::path(args.out_dir) / "raw" / "threshold_raw.tsv") << "\n";
        std::cerr << "Wrote " << (std::filesystem::path(args.out_dir) / "raw" / "threshold_by_ed.tsv") << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "threshold_bench: " << e.what() << "\n";
        return 1;
    }
}

#include "dataset.hpp"
#include "method_interface.hpp"
#include "perf_schema.hpp"
#include "timing.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

volatile uint64_t g_pass_sink = 0;

struct Options {
  std::string dataset = "benchmark/data/generated/pairs_len100_ed0_15.tsv";
  std::vector<std::string> methods = {"leap_avx2", "leap_avx512", "lv89",
                                      "miniwfa", "wfa2_fresh", "wfa2_reuse"};
  std::vector<int> ks = {1, 2, 3, 4, 5};
  int length = 100;
  int true_ed_min = 0;
  int true_ed_max = 15;
  std::string cache_mode = "warm";
  std::string out_dir = "benchmark/results";
  uint64_t seed = 1;
};

bool is_option(const char* s) { return std::string(s).rfind("--", 0) == 0; }

std::vector<std::string> parse_strings(int argc, char** argv,
                                       const std::string& name,
                                       std::vector<std::string> def) {
  for (int i = 1; i < argc; ++i) {
    if (argv[i] != name) continue;
    std::vector<std::string> out;
    for (++i; i < argc && !is_option(argv[i]); ++i) out.emplace_back(argv[i]);
    return out.empty() ? def : out;
  }
  return def;
}

std::vector<int> parse_ints(int argc, char** argv, const std::string& name,
                            std::vector<int> def) {
  for (int i = 1; i < argc; ++i) {
    if (argv[i] != name) continue;
    std::vector<int> out;
    for (++i; i < argc && !is_option(argv[i]); ++i) out.push_back(std::stoi(argv[i]));
    return out.empty() ? def : out;
  }
  return def;
}

std::string parse_string(int argc, char** argv, const std::string& name,
                         const std::string& def) {
  for (int i = 1; i + 1 < argc; ++i)
    if (argv[i] == name) return argv[i + 1];
  return def;
}

int parse_int(int argc, char** argv, const std::string& name, int def) {
  for (int i = 1; i + 1 < argc; ++i)
    if (argv[i] == name) return std::stoi(argv[i + 1]);
  return def;
}

Options parse_options(int argc, char** argv) {
  Options o;
  o.dataset = parse_string(argc, argv, "--dataset", o.dataset);
  o.methods = parse_strings(argc, argv, "--methods", o.methods);
  o.ks = parse_ints(argc, argv, "--ks", o.ks);
  auto lengths = parse_ints(argc, argv, "--lengths", {o.length});
  o.length = lengths.front();
  o.true_ed_min = parse_int(argc, argv, "--true-ed-min", o.true_ed_min);
  o.true_ed_max = parse_int(argc, argv, "--true-ed-max", o.true_ed_max);
  o.cache_mode = parse_string(argc, argv, "--cache-mode", o.cache_mode);
  o.out_dir = parse_string(argc, argv, "--out-dir", o.out_dir);
  o.seed = uint64_t(parse_int(argc, argv, "--seed", int(o.seed)));
  return o;
}

std::vector<bench::MethodSpec> all_methods() {
  return {
      {"leap_avx2", "avx2", "forward", bench::verify_leap_avx2,
       bench::method_available_avx2, nullptr, nullptr},
      {"leap_avx512", "avx512", "forward", bench::verify_leap_avx512,
       bench::method_available_avx512, nullptr, nullptr},
      {"lv89", "lv89", "default", bench::verify_lv89, nullptr, nullptr, nullptr},
      {"miniwfa", "miniwfa", "default", bench::verify_miniwfa, nullptr, nullptr,
       nullptr},
      {"wfa2_fresh", "wfa2", "fresh", bench::verify_wfa2_fresh, nullptr, nullptr,
       nullptr},
      {"wfa2_reuse", "wfa2", "reuse", bench::verify_wfa2_reuse, nullptr,
       bench::method_prepare_wfa2_reuse, bench::method_cleanup_wfa2_reuse},
  };
}

void touch_rows(const std::vector<const bench::PairRecord*>& rows) {
  volatile uint64_t sink = 0;
  for (const auto* r : rows) {
    for (char c : r->read) sink += uint8_t(c);
    for (char c : r->reference) sink += uint8_t(c);
  }
  g_pass_sink += sink & 1u;
}

void evict_cache() {
  static std::vector<uint8_t> eviction(256ull * 1024ull * 1024ull, 1);
  volatile uint64_t sink = 0;
  for (size_t i = 0; i < eviction.size(); i += 64) sink += eviction[i];
  g_pass_sink += sink & 1u;
}

void warmup(const bench::MethodSpec& method,
            const std::vector<const bench::PairRecord*>& rows, int k) {
  touch_rows(rows);
  for (const auto* r : rows) {
    bench::VerifyInput input{r->read.data(), r->reference.data(), r->length, k};
    const auto result = method.verify(input);
    g_pass_sink += result.pass ? 1 : 0;
  }
}

struct RunStats {
  uint64_t total_cpu_ns = 0;
  uint64_t pass_count = 0;
  uint64_t mismatch_count = 0;
  uint64_t sink = 0;
};

RunStats run_timed(const bench::MethodSpec& method,
                   const std::vector<const bench::PairRecord*>& rows, int k,
                   const std::string& cache_mode) {
  if (method.method == "leap_avx2" || method.method == "leap_avx512") {
    const auto leap_stats = method.method == "leap_avx2"
                                ? bench::run_leap_avx2_forward_timed(
                                      rows, k, cache_mode, &g_pass_sink)
                                : bench::run_leap_avx512_forward_timed(
                                      rows, k, cache_mode, &g_pass_sink);
    return {leap_stats.total_cpu_ns, leap_stats.pass_count,
            leap_stats.mismatch_count, leap_stats.sink};
  }

  if (cache_mode == "warm") {
    warmup(method, rows, k);
  } else if (cache_mode == "cold-ish") {
    evict_cache();
  } else {
    throw std::runtime_error("unknown cache mode: " + cache_mode);
  }

  std::vector<uint8_t> measured(rows.size());
  uint64_t local_sink = 0;
  const uint64_t start = bench::thread_cpu_time_ns();
  for (size_t i = 0; i < rows.size(); ++i) {
    const auto* r = rows[i];
    bench::VerifyInput input{r->read.data(), r->reference.data(), r->length, k};
    const auto result = method.verify(input);
    measured[i] = result.pass ? 1 : 0;
    local_sink += measured[i];
    g_pass_sink += measured[i];
  }
  const uint64_t end = bench::thread_cpu_time_ns();

  RunStats stats;
  stats.total_cpu_ns = end - start;
  stats.pass_count = local_sink;
  stats.sink = g_pass_sink;
  for (size_t i = 0; i < rows.size(); ++i) {
    if (measured[i] != expected_pass_label(*rows[i], k)) ++stats.mismatch_count;
  }
  return stats;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options opt = parse_options(argc, argv);
    const auto dataset = bench::load_dataset_tsv(opt.dataset);

    std::vector<const bench::PairRecord*> selected;
    std::map<int, std::vector<const bench::PairRecord*>> by_ed;
    for (const auto& row : dataset) {
      if (row.length != opt.length) continue;
      if (row.true_ed < opt.true_ed_min || row.true_ed > opt.true_ed_max) continue;
      selected.push_back(&row);
      by_ed[row.true_ed].push_back(&row);
    }
    if (selected.empty()) throw std::runtime_error("no rows selected from dataset");

    std::filesystem::create_directories(opt.out_dir + "/raw");
    std::ofstream raw(opt.out_dir + "/raw/threshold_raw.tsv");
    std::ofstream grouped(opt.out_dir + "/raw/threshold_by_ed.tsv");
    raw << bench::kThresholdRawHeader;
    grouped << bench::kThresholdByEdHeader;

    std::unordered_map<std::string, bench::MethodSpec> registry;
    for (auto& m : all_methods()) registry.emplace(m.method, m);

    for (const auto& method_name : opt.methods) {
      if (!registry.count(method_name)) {
        throw std::runtime_error("unknown method: " + method_name);
      }
      const auto method = registry.at(method_name);
      if (method.available && !method.available()) {
        throw std::runtime_error(method.method + " requested but backend is unavailable");
      }
      if (method.prepare && !method.prepare()) {
        throw std::runtime_error("failed to prepare method: " + method.method);
      }
      for (const int k : opt.ks) {
        const auto total = run_timed(method, selected, k, opt.cache_mode);
        const uint64_t fail_count = selected.size() - total.pass_count;
        const double avg = double(total.total_cpu_ns) / double(selected.size());
        raw << method.method << '\t' << method.backend << '\t' << method.mode
            << '\t' << opt.cache_mode << '\t' << opt.length << '\t' << k << '\t'
            << selected.size() << '\t' << total.pass_count << '\t' << fail_count
            << '\t' << total.mismatch_count << '\t' << total.total_cpu_ns << '\t'
            << avg << '\t' << (avg * 10000000.0 / 1.0e9) << '\t'
            << total.sink << '\t' << opt.seed << '\n';

        for (const auto& [ed, rows] : by_ed) {
          const auto stats = run_timed(method, rows, k, opt.cache_mode);
          const uint64_t ed_fail_count = rows.size() - stats.pass_count;
          const double ed_avg = double(stats.total_cpu_ns) / double(rows.size());
          grouped << method.method << '\t' << method.backend << '\t' << method.mode
                  << '\t' << opt.cache_mode << '\t' << opt.length << '\t' << k
                  << '\t' << ed << '\t' << rows.size() << '\t' << stats.pass_count
                  << '\t' << ed_fail_count << '\t' << stats.mismatch_count << '\t'
                  << stats.total_cpu_ns << '\t' << ed_avg << '\t' << opt.seed
                  << '\n';
        }
      }
      if (method.cleanup) method.cleanup();
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "threshold_bench: " << e.what() << "\n";
    return 1;
  }
}


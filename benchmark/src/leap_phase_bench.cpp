#include "dataset.hpp"
#include "leap_phase_api.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool is_option(const char* s) { return std::string(s).rfind("--", 0) == 0; }

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

void write_row(std::ofstream& os, const bench::LeapPhaseRow& r) {
  const uint64_t total = r.construct_init_cpu_ns + r.load_reads_cpu_ns +
                         r.calculate_masks_cpu_ns + r.reset_cpu_ns +
                         r.run_cpu_ns + r.check_pass_cpu_ns + r.sink_cpu_ns;
  const auto pct = [total](uint64_t v) {
    return total == 0 ? 0.0 : 100.0 * double(v) / double(total);
  };
  os << r.backend << '\t' << r.length << '\t' << r.k << '\t'
     << r.true_ed_group << '\t' << r.pair_count << '\t' << r.chunk_size << '\t'
     << r.chunk_count_per_pair << '\t' << r.construct_init_cpu_ns << '\t'
     << r.load_reads_cpu_ns << '\t' << r.calculate_masks_cpu_ns << '\t'
     << r.reset_cpu_ns << '\t' << r.run_cpu_ns << '\t' << r.check_pass_cpu_ns
     << '\t' << r.sink_cpu_ns << '\t' << total << '\t'
     << pct(r.construct_init_cpu_ns) << '\t' << pct(r.load_reads_cpu_ns) << '\t'
     << pct(r.calculate_masks_cpu_ns) << '\t' << pct(r.reset_cpu_ns) << '\t'
     << pct(r.run_cpu_ns) << '\t' << pct(r.check_pass_cpu_ns) << '\t'
     << pct(r.sink_cpu_ns) << '\t'
     << (r.pair_count ? double(total) / double(r.pair_count) : 0.0) << '\t'
     << (r.pair_count ? double(r.run_cpu_ns) / double(r.pair_count) : 0.0)
     << '\t' << r.seed << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string dataset_path = parse_string(
        argc, argv, "--dataset", "benchmark/data/generated/pairs_len100_ed0_15.tsv");
    const std::string out_dir = parse_string(argc, argv, "--out-dir",
                                             "benchmark/results");
    const auto ks = parse_ints(argc, argv, "--ks", {1, 2, 3, 4, 5});
    const auto lengths = parse_ints(argc, argv, "--lengths", {100});
    const int length = lengths.front();
    const int ed_min = parse_int(argc, argv, "--true-ed-min", 0);
    const int ed_max = parse_int(argc, argv, "--true-ed-max", 15);
    const uint64_t seed = uint64_t(parse_int(argc, argv, "--seed", 1));
    const auto backends =
        parse_strings(argc, argv, "--backends", {"leap_avx2", "leap_avx512"});

    const auto dataset = bench::load_dataset_tsv(dataset_path);
    std::vector<const bench::PairRecord*> rows;
    for (const auto& row : dataset) {
      if (row.length == length && row.true_ed >= ed_min && row.true_ed <= ed_max) {
        rows.push_back(&row);
      }
    }
    if (rows.empty()) throw std::runtime_error("no rows selected from dataset");

    std::filesystem::create_directories(out_dir + "/raw");
    std::ofstream os(out_dir + "/raw/leap_phase_raw.tsv");
    os << "backend\tlength\tk\ttrue_ed_group\tpair_count\tchunk_size\t"
          "chunk_count_per_pair\tconstruct_init_cpu_ns\tload_reads_cpu_ns\t"
          "calculate_masks_cpu_ns\treset_cpu_ns\trun_cpu_ns\tcheck_pass_cpu_ns\t"
          "sink_cpu_ns\ttotal_cpu_ns\tconstruct_init_pct\tload_reads_pct\t"
          "calculate_masks_pct\treset_pct\trun_pct\tcheck_pass_pct\tsink_pct\t"
          "avg_total_cpu_ns_per_pair\tavg_run_cpu_ns_per_pair\tseed\n";

    for (const auto& backend : backends) {
      std::vector<bench::LeapPhaseRow> result;
      if (backend == "leap_avx2") {
        result = bench::run_leap_phase_avx2(rows, ks, ed_min, ed_max, seed);
      } else if (backend == "leap_avx512") {
        result = bench::run_leap_phase_avx512(rows, ks, ed_min, ed_max, seed);
      } else {
        throw std::runtime_error("unknown LEAP phase backend: " + backend);
      }
      for (const auto& row : result) write_row(os, row);
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "leap_phase_bench: " << e.what() << "\n";
    return 1;
  }
}


#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct PairRecord {
  std::string pair_id;
  std::string read;
  std::string reference;
  int ground_truth_ed = -1;
  int max_edit = -1;
};

struct BenchOptions {
  std::string input;
  std::string dataset;
  std::string method;
  std::string backend;
};

struct BenchResult {
  uint64_t total_time_ns = 0;
  uint64_t correct_count = 0;
  uint64_t mismatch_count = 0;
};

inline std::vector<std::string> split_tab(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  std::stringstream ss(line);
  while (std::getline(ss, field, '\t')) fields.push_back(field);
  return fields;
}

inline std::vector<PairRecord> load_pairs_tsv(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("failed to open input TSV: " + path);

  std::string line;
  if (!std::getline(in, line)) throw std::runtime_error("empty input TSV: " + path);
  const auto header = split_tab(line);
  int id_i = -1, read_i = -1, ref_i = -1, gt_i = -1, max_i = -1;
  for (int i = 0; i < static_cast<int>(header.size()); ++i) {
    if (header[i] == "pair_id") id_i = i;
    else if (header[i] == "read") read_i = i;
    else if (header[i] == "reference") ref_i = i;
    else if (header[i] == "ground_truth_ed") gt_i = i;
    else if (header[i] == "max_edit") max_i = i;
  }
  if (id_i < 0 || read_i < 0 || ref_i < 0 || gt_i < 0 || max_i < 0) {
    throw std::runtime_error("input TSV missing required columns");
  }

  std::vector<PairRecord> pairs;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const auto f = split_tab(line);
    const int need = std::max(std::max(id_i, read_i), std::max(std::max(ref_i, gt_i), max_i));
    if (static_cast<int>(f.size()) <= need) throw std::runtime_error("malformed TSV row: " + line);
    PairRecord p;
    p.pair_id = f[id_i];
    p.read = f[read_i];
    p.reference = f[ref_i];
    p.ground_truth_ed = std::stoi(f[gt_i]);
    p.max_edit = std::stoi(f[max_i]);
    pairs.push_back(std::move(p));
  }
  return pairs;
}

inline BenchOptions parse_bench_options(int argc, char** argv, const std::string& method,
                                        const std::string& backend) {
  BenchOptions opt;
  opt.method = method;
  opt.backend = backend;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--input" && i + 1 < argc) opt.input = argv[++i];
    else if (arg == "--dataset" && i + 1 < argc) opt.dataset = argv[++i];
    else if (arg == "--backend" && i + 1 < argc) opt.backend = argv[++i];
    else throw std::runtime_error("unknown or incomplete argument: " + arg);
  }
  if (opt.input.empty()) throw std::runtime_error("--input is required");
  if (opt.dataset.empty()) opt.dataset = opt.input;
  return opt;
}

template <typename Fn>
inline int run_wrapper_main(int argc, char** argv, const std::string& method,
                            const std::string& backend, Fn&& distance_fn) {
  try {
    const BenchOptions opt = parse_bench_options(argc, argv, method, backend);
    const auto pairs = load_pairs_tsv(opt.input);
    BenchResult result;
    int max_edit = pairs.empty() ? -1 : pairs.front().max_edit;

    std::vector<std::string> mismatches;
    for (const auto& p : pairs) {
      if (p.max_edit != max_edit) throw std::runtime_error("mixed max_edit values in dataset");
      const auto t0 = std::chrono::steady_clock::now();
      const int ed = distance_fn(p.read, p.reference, p.max_edit);
      const auto t1 = std::chrono::steady_clock::now();
      result.total_time_ns += static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
      if (ed == p.ground_truth_ed) {
        ++result.correct_count;
      } else {
        ++result.mismatch_count;
        std::ostringstream os;
        os << p.pair_id << '\t' << method << '\t' << opt.backend << '\t' << p.read << '\t'
           << p.reference << '\t' << p.ground_truth_ed << '\t' << ed;
        mismatches.push_back(os.str());
      }
    }

    const uint64_t n = pairs.size();
    const double avg = n ? static_cast<double>(result.total_time_ns) / static_cast<double>(n) : 0.0;
    std::cout << "method\tbackend\tdataset\tmax_edit\tnum_pairs\ttotal_time_ns\t"
                 "avg_time_ns_per_pair\tcorrect_count\tmismatch_count\n";
    std::cout << method << '\t' << opt.backend << '\t' << opt.dataset << '\t' << max_edit << '\t'
              << n << '\t' << result.total_time_ns << '\t' << avg << '\t'
              << result.correct_count << '\t' << result.mismatch_count << '\n';
    if (!mismatches.empty()) {
      std::cerr << "pair_id\tmethod\tbackend\tread\treference\tground_truth\tmethod_result\n";
      for (const auto& row : mismatches) std::cerr << row << '\n';
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
  }
}

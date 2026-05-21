#pragma once

#include "bench_common.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bench {

inline std::vector<std::string> split_tab(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  std::stringstream ss(line);
  while (std::getline(ss, field, '\t')) fields.push_back(field);
  return fields;
}

inline std::vector<PairRecord> load_dataset_tsv(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("failed to open dataset: " + path);
  std::string line;
  if (!std::getline(in, line)) throw std::runtime_error("empty dataset: " + path);
  std::vector<PairRecord> rows;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    const auto f = split_tab(line);
    if (f.size() != 15) throw std::runtime_error("bad TSV column count in dataset");
    PairRecord r;
    r.pair_id = std::stoull(f[0]);
    r.length = std::stoi(f[1]);
    r.true_ed = std::stoi(f[2]);
    r.read = f[3];
    r.reference = f[4];
    r.num_sub = std::stoi(f[5]);
    r.num_ins = std::stoi(f[6]);
    r.num_del = std::stoi(f[7]);
    r.edit_pattern = f[8];
    r.seed = std::stoull(f[9]);
    for (int k = 1; k <= 5; ++k) r.pass[k] = uint8_t(std::stoi(f[9 + k]));
    rows.push_back(std::move(r));
  }
  return rows;
}

}  // namespace bench


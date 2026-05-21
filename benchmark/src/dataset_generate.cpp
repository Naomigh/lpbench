#include "bench_common.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

char random_base(std::mt19937_64& rng) {
  static constexpr char bases[] = {'A', 'C', 'G', 'T'};
  return bases[std::uniform_int_distribution<int>(0, 3)(rng)];
}

char different_base(char c, std::mt19937_64& rng) {
  char b = random_base(rng);
  while (b == c) b = random_base(rng);
  return b;
}

std::string random_dna(int length, std::mt19937_64& rng) {
  std::string s;
  s.reserve(length);
  for (int i = 0; i < length; ++i) s.push_back(random_base(rng));
  return s;
}

struct Candidate {
  std::string read;
  int num_sub = 0;
  int num_ins = 0;
  int num_del = 0;
  std::string pattern;
};

void apply_subs(std::string& read, int count, std::mt19937_64& rng) {
  std::vector<int> positions(read.size());
  for (int i = 0; i < int(read.size()); ++i) positions[i] = i;
  std::shuffle(positions.begin(), positions.end(), rng);
  for (int i = 0; i < count; ++i) {
    const int pos = positions[i % positions.size()];
    read[pos] = different_base(read[pos], rng);
  }
}

bool apply_balanced_indels(std::string& read, int pairs, int final_length,
                           std::mt19937_64& rng) {
  if (pairs <= 0) return true;
  for (int i = 0; i < pairs; ++i) {
    if (read.empty()) return false;
    int del = std::uniform_int_distribution<int>(0, int(read.size()) - 1)(rng);
    read.erase(read.begin() + del);
    int ins = std::uniform_int_distribution<int>(0, int(read.size()))(rng);
    read.insert(read.begin() + ins, random_base(rng));
  }
  return int(read.size()) == final_length;
}

Candidate make_candidate(const std::string& ref, int target_ed, int variant,
                         std::mt19937_64& rng) {
  Candidate c;
  c.read = ref;
  if (target_ed == 0) {
    c.pattern = "exact";
    return c;
  }
  if (variant % 3 == 0 || target_ed == 1) {
    c.num_sub = target_ed;
    c.pattern = "sub_only";
    apply_subs(c.read, c.num_sub, rng);
    return c;
  }
  if (variant % 3 == 1 && target_ed >= 2) {
    c.num_ins = target_ed / 2;
    c.num_del = target_ed / 2;
    c.pattern = "indel_balanced";
    apply_balanced_indels(c.read, c.num_ins, ref.size(), rng);
    if (target_ed % 2) {
      c.num_sub = 1;
      apply_subs(c.read, 1, rng);
      c.pattern = "mixed";
    }
    return c;
  }
  c.num_ins = std::max(1, target_ed / 3);
  c.num_del = c.num_ins;
  c.num_sub = std::max(0, target_ed - 2 * c.num_ins);
  c.pattern = "mixed";
  apply_balanced_indels(c.read, c.num_ins, ref.size(), rng);
  if (c.num_sub > 0) apply_subs(c.read, c.num_sub, rng);
  return c;
}

int parse_int_arg(int argc, char** argv, const std::string& name, int def) {
  for (int i = 1; i + 1 < argc; ++i)
    if (argv[i] == name) return std::stoi(argv[i + 1]);
  return def;
}

std::string parse_string_arg(int argc, char** argv, const std::string& name,
                             const std::string& def) {
  for (int i = 1; i + 1 < argc; ++i)
    if (argv[i] == name) return argv[i + 1];
  return def;
}

}  // namespace

int main(int argc, char** argv) {
  const int length = parse_int_arg(argc, argv, "--length", 100);
  const int ed_min = parse_int_arg(argc, argv, "--true-ed-min", 0);
  const int ed_max = parse_int_arg(argc, argv, "--true-ed-max", 15);
  const int pairs_per_ed = parse_int_arg(argc, argv, "--pairs-per-ed", 10000);
  const uint64_t seed = uint64_t(parse_int_arg(argc, argv, "--seed", 1));
  const std::string out = parse_string_arg(
      argc, argv, "--out", "benchmark/data/generated/pairs_len100_ed0_15.tsv");

  std::filesystem::create_directories(std::filesystem::path(out).parent_path());
  std::ofstream os(out);
  if (!os) throw std::runtime_error("failed to write " + out);
  os << "pair_id\tlength\ttrue_ed\tread\treference\tnum_sub\tnum_ins\tnum_del\t"
        "edit_pattern\tseed\tpass_k1\tpass_k2\tpass_k3\tpass_k4\tpass_k5\n";

  uint64_t pair_id = 0;
  std::mt19937_64 rng(seed);
  for (int ed = ed_min; ed <= ed_max; ++ed) {
    int accepted = 0;
    uint64_t attempts = 0;
    while (accepted < pairs_per_ed) {
      if (++attempts > uint64_t(pairs_per_ed) * 5000ull) {
        throw std::runtime_error("too many rejected generated pairs");
      }
      const std::string ref = random_dna(length, rng);
      Candidate c = make_candidate(ref, ed, accepted + int(attempts), rng);
      if (int(c.read.size()) != length) continue;
      const int verified =
          bench::trusted_edit_distance(c.read.data(), length, ref.data(), length);
      if (verified != ed) continue;
      os << pair_id++ << '\t' << length << '\t' << ed << '\t' << c.read << '\t'
         << ref << '\t' << c.num_sub << '\t' << c.num_ins << '\t' << c.num_del
         << '\t' << c.pattern << '\t' << seed;
      for (int k = 1; k <= 5; ++k) os << '\t' << (ed <= k ? 1 : 0);
      os << '\n';
      ++accepted;
    }
  }
  std::cerr << "wrote " << pair_id << " pairs to " << out << "\n";
  return 0;
}


#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <time.h>

#if defined(METHOD_LEAP_AVX2) || defined(METHOD_LEAP_AVX512)
#include "SIMD_ED.h"
#endif

#ifdef METHOD_MINIWFA
extern "C" {
#include "miniwfa.h"
}
#endif

#ifdef METHOD_LV89
extern "C" {
#include "lv89.h"
}
#endif

#ifdef METHOD_WFA2
extern "C" {
#include "wavefront/wavefront_align.h"
}
#endif

struct Pair {
  std::string read;
  std::string reference;
  int edit_distance = 0;
  bool ground_truth_pass = true;
};

struct Group {
  int length = 0;
  int k = 0;
  std::vector<Pair> pairs;
};

struct ResultRow {
  std::string method;
  int length = 0;
  int k = 0;
  int pair_count = 0;
  int pass_count = 0;
  int fail_count = 0;
  uint64_t total_cpu_ns = 0;
  double avg_cpu_ns_per_pair = 0.0;
  double seconds_per_10m_pairs = 0.0;
  int mismatch_count = 0;
};

static volatile uint64_t g_touch_sink = 0;
static volatile uint64_t g_result_sink = 0;

static uint64_t thread_cpu_time_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static int trusted_edit_distance(const std::string& a, const std::string& b) {
  std::vector<int> prev(b.size() + 1), cur(b.size() + 1);
  for (size_t j = 0; j <= b.size(); ++j) prev[j] = (int)j;
  for (size_t i = 1; i <= a.size(); ++i) {
    cur[0] = (int)i;
    for (size_t j = 1; j <= b.size(); ++j) {
      const int sub = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
      const int del = prev[j] + 1;
      const int ins = cur[j - 1] + 1;
      cur[j] = std::min(sub, std::min(del, ins));
    }
    std::swap(prev, cur);
  }
  return prev[b.size()];
}

static char random_base(std::mt19937_64& rng) {
  static const char bases[] = {'A', 'C', 'G', 'T'};
  return bases[std::uniform_int_distribution<int>(0, 3)(rng)];
}

static char different_base(char c, std::mt19937_64& rng) {
  char b = random_base(rng);
  while (b == c) b = random_base(rng);
  return b;
}

static Pair generate_pair(int length, int k, std::mt19937_64& rng) {
  for (;;) {
    Pair p;
    p.reference.resize(length);
    for (char& c : p.reference) c = random_base(rng);
    p.read = p.reference;

    const int target_ed = std::uniform_int_distribution<int>(0, k)(rng);
    int used = 0;
    while (used < target_ed) {
      const bool can_indel_pair = used + 2 <= target_ed;
      const bool use_indel_pair = can_indel_pair && std::uniform_int_distribution<int>(0, 2)(rng) == 0;
      if (use_indel_pair) {
        const int ins_pos = std::uniform_int_distribution<int>(0, (int)p.read.size())(rng);
        p.read.insert(p.read.begin() + ins_pos, random_base(rng));
        const int del_pos = std::uniform_int_distribution<int>(0, (int)p.read.size() - 1)(rng);
        p.read.erase(p.read.begin() + del_pos);
        used += 2;
      } else {
        const int pos = std::uniform_int_distribution<int>(0, length - 1)(rng);
        p.read[pos] = different_base(p.read[pos], rng);
        ++used;
      }
    }

    if ((int)p.read.size() != length || (int)p.reference.size() != length) continue;
    p.edit_distance = trusted_edit_distance(p.read, p.reference);
    p.ground_truth_pass = p.edit_distance <= k;
    if (p.ground_truth_pass) return p;
  }
}

static std::vector<Group> generate_dataset(int pairs_per_group) {
  std::vector<Group> groups;
  std::mt19937_64 rng(0x5eed1234567890abULL);
  const int lengths[] = {100, 200, 500};
  for (int length : lengths) {
    for (int k = 1; k <= 5; ++k) {
      Group g;
      g.length = length;
      g.k = k;
      g.pairs.reserve(pairs_per_group);
      for (int i = 0; i < pairs_per_group; ++i) {
        g.pairs.push_back(generate_pair(length, k, rng));
      }
      groups.push_back(std::move(g));
    }
  }
  return groups;
}

static void touch_all_bytes(const std::vector<Group>& groups) {
  uint64_t sink = 0;
  for (const Group& g : groups) {
    for (const Pair& p : g.pairs) {
      for (unsigned char c : p.read) sink += c;
      for (unsigned char c : p.reference) sink += c;
    }
  }
  g_touch_sink += sink;
}

static bool cpu_has_avx512() {
#if defined(__GNUC__) && defined(__x86_64__)
  __builtin_cpu_init();
  return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") &&
         __builtin_cpu_supports("avx512vl");
#else
  return false;
#endif
}

static const char* method_name() {
#ifdef METHOD_LEAP_AVX2
  return "leap_avx2";
#elif defined(METHOD_LEAP_AVX512)
  return "leap_avx512";
#elif defined(METHOD_MINIWFA)
  return "miniwfa";
#elif defined(METHOD_LV89)
  return "lv89";
#elif defined(METHOD_WFA2)
  return "wfa2-lib";
#else
#error "No METHOD_* macro defined"
#endif
}

static bool verify_pair(const Pair& pair, int k) {
#if defined(METHOD_LEAP_AVX2) || defined(METHOD_LEAP_AVX512)
#ifdef METHOD_LEAP_AVX512
  const int max_length = LEAP_AVX512_EFFECTIVE_LENGTH;
#else
  const int max_length = LEAP_AVX_EFFECTIVE_LENGTH;
#endif
  const int length = (int)pair.read.size();
  bool all_chunks_pass = true;
  for (int offset = 0; offset < length; offset += max_length) {
    const int chunk_len = std::min(max_length, length - offset);
    SIMD_ED ed;
    ed.init_levenshtein(k, ED_GLOBAL, false);
    ed.load_reads((char*)pair.read.data() + offset, (char*)pair.reference.data() + offset, chunk_len);
    ed.calculate_masks();
    ed.reset();
    ed.run();
    all_chunks_pass = all_chunks_pass && ed.check_pass();
  }
  return all_chunks_pass;
#elif defined(METHOD_MINIWFA)
  mwf_opt_t opt;
  mwf_opt_init(&opt);
  opt.flag &= ~MWF_F_CIGAR;
  opt.x = 1;
  opt.o1 = 0;
  opt.e1 = 1;
  opt.o2 = 0;
  opt.e2 = 1;
  opt.max_s = k;
  mwf_rst_t rst;
  std::memset(&rst, 0, sizeof(rst));
  mwf_wfa_exact(nullptr, &opt, (int32_t)pair.reference.size(), pair.reference.data(),
                (int32_t)pair.read.size(), pair.read.data(), &rst);
  free(rst.cigar);
  return rst.s >= 0 && rst.s <= k;
#elif defined(METHOD_LV89)
  int32_t score = 0;
  int32_t t_endl = 0;
  int32_t q_endl = 0;
  lv_ed((int32_t)pair.reference.size(), pair.reference.data(),
        (int32_t)pair.read.size(), pair.read.data(), 0, k, 0,
        &score, &t_endl, &q_endl, nullptr);
  return score <= k && t_endl == (int32_t)pair.reference.size() &&
         q_endl == (int32_t)pair.read.size();
#elif defined(METHOD_WFA2)
  wavefront_aligner_attr_t attributes = wavefront_aligner_attr_default;
  attributes.distance_metric = edit;
  attributes.alignment_scope = compute_score;
  attributes.alignment_form.span = alignment_end2end;
  attributes.heuristic.strategy = wf_heuristic_none;
  attributes.system.max_num_threads = 1;
  wavefront_aligner_t* wf = wavefront_aligner_new(&attributes);
  const int status = wavefront_align(wf, pair.read.data(), (int)pair.read.size(),
                                     pair.reference.data(), (int)pair.reference.size());
  const int score = wf->align_status.score;
  wavefront_aligner_delete(wf);
  return status == WF_STATUS_ALG_COMPLETED && score <= k;
#endif
}

static ResultRow run_group(const Group& group, int prefetch_distance, std::vector<uint8_t>& measured_pass) {
  measured_pass.assign(group.pairs.size(), 0);

  for (const Pair& p : group.pairs) {
    g_result_sink += verify_pair(p, group.k) ? 1 : 0;
  }

  const uint64_t start_ns = thread_cpu_time_ns();
  int pass_count = 0;
  for (size_t i = 0; i < group.pairs.size(); ++i) {
    if (prefetch_distance > 0) {
      const size_t pf = i + (size_t)prefetch_distance;
      if (pf < group.pairs.size()) {
        __builtin_prefetch(group.pairs[pf].read.data(), 0, 3);
        __builtin_prefetch(group.pairs[pf].reference.data(), 0, 3);
      }
    }
    const bool pass = verify_pair(group.pairs[i], group.k);
    measured_pass[i] = pass ? 1 : 0;
    pass_count += pass ? 1 : 0;
  }
  const uint64_t end_ns = thread_cpu_time_ns();

  int mismatches = 0;
  for (size_t i = 0; i < group.pairs.size(); ++i) {
    if ((measured_pass[i] != 0) != group.pairs[i].ground_truth_pass) ++mismatches;
  }

  ResultRow row;
  row.method = method_name();
  row.length = group.length;
  row.k = group.k;
  row.pair_count = (int)group.pairs.size();
  row.pass_count = pass_count;
  row.fail_count = row.pair_count - pass_count;
  row.total_cpu_ns = end_ns - start_ns;
  row.avg_cpu_ns_per_pair = (double)row.total_cpu_ns / (double)row.pair_count;
  row.seconds_per_10m_pairs = row.avg_cpu_ns_per_pair / 100.0;
  row.mismatch_count = mismatches;
  g_result_sink += pass_count;
  return row;
}

static void ensure_parent_dirs() {
  mkdir("benchmark/results", 0775);
  mkdir("benchmark/results/raw", 0775);
  mkdir("benchmark/results/summary", 0775);
}

static std::string arg_value(int argc, char** argv, const char* name, const char* default_value) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  return default_value;
}

static int arg_int(int argc, char** argv, const char* name, int default_value) {
  return std::atoi(arg_value(argc, argv, name, std::to_string(default_value).c_str()).c_str());
}

int main(int argc, char** argv) {
#ifdef METHOD_LEAP_AVX512
  if (!cpu_has_avx512()) {
    std::cerr << "leap_avx512 requires AVX-512F/BW/VL on this CPU\n";
    return 77;
  }
#endif
  const int pairs_per_group = arg_int(argc, argv, "--pairs-per-group", 1000);
  const int prefetch_distance = arg_int(argc, argv, "--prefetch-distance", 16);
  const std::string raw_out = arg_value(argc, argv, "--raw-out", "benchmark/results/raw/threshold_verify_raw.tsv");
  const std::string mismatch_out = arg_value(argc, argv, "--mismatch-out", "benchmark/results/raw/threshold_verify_mismatches.tsv");
  const bool append = arg_int(argc, argv, "--append", 0) != 0;

  ensure_parent_dirs();
  std::vector<Group> groups = generate_dataset(pairs_per_group);
  touch_all_bytes(groups);

  std::ofstream raw(raw_out, append ? std::ios::app : std::ios::out);
  std::ofstream mismatches(mismatch_out, append ? std::ios::app : std::ios::out);
  if (!append) {
    raw << "method\tlength\tk\tpair_count\tpass_count\tfail_count\ttotal_cpu_ns\tavg_cpu_ns_per_pair\tseconds_per_10M_pairs\tmismatch_count\tprefetch_distance\n";
    mismatches << "method\tlength\tk\tpair_index\tground_truth_pass\tobserved_pass\ttrue_edit_distance\tread\treference\n";
  }

  for (const Group& group : groups) {
    std::vector<uint8_t> measured_pass;
    ResultRow row = run_group(group, prefetch_distance, measured_pass);
    raw << row.method << '\t' << row.length << '\t' << row.k << '\t' << row.pair_count
        << '\t' << row.pass_count << '\t' << row.fail_count << '\t' << row.total_cpu_ns
        << '\t' << row.avg_cpu_ns_per_pair << '\t' << row.seconds_per_10m_pairs
        << '\t' << row.mismatch_count << '\t' << prefetch_distance << '\n';

    int written = 0;
    for (size_t i = 0; i < group.pairs.size() && written < 10; ++i) {
      const bool observed = measured_pass[i] != 0;
      if (observed != group.pairs[i].ground_truth_pass) {
        mismatches << row.method << '\t' << row.length << '\t' << row.k << '\t' << i
                   << '\t' << (group.pairs[i].ground_truth_pass ? 1 : 0)
                   << '\t' << (observed ? 1 : 0)
                   << '\t' << group.pairs[i].edit_distance
                   << '\t' << group.pairs[i].read
                   << '\t' << group.pairs[i].reference << '\n';
        ++written;
      }
    }
  }
  return 0;
}

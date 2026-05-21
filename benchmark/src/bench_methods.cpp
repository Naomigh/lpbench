#include "leap_backend_api.h"

#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include "lv89.h"
#include "miniwfa.h"
#include "wavefront/wavefront_align.h"
}

namespace {

static volatile uint64_t g_pass_sink = 0;
static volatile uint64_t g_score_sink = 0;

uint64_t thread_cpu_time_ns() {
  struct timespec ts;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
  return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}

struct Pair {
  std::string id;
  int length = 0;
  int true_ed = 0;
  std::string read;
  std::string reference;
};

struct Options {
  std::string dataset;
  std::string method;
  std::string timing_mode = "batch";
  int length = 0;
  int k = -1;
  size_t pairs = 0;
  size_t n1 = 0;
  size_t n2 = 0;
  int warmup_passes = 0;
  uint64_t seed = 0;
};

struct Measurement {
  uint64_t time_ns = 0;
  uint64_t pass_sink = 0;
  uint64_t score_sink = 0;
  uint64_t timing_batches = 1;
  std::string notes;
};

[[noreturn]] void fail(const std::string& message) {
  throw std::runtime_error(message);
}

long long parse_signed(const std::string& text, const std::string& name) {
  char* end = nullptr;
  errno = 0;
  const long long value = std::strtoll(text.c_str(), &end, 10);
  if (errno || end == text.c_str() || *end != '\0') {
    fail("invalid integer for " + name + ": " + text);
  }
  return value;
}

void usage(std::ostream& out) {
  out << "bench_methods --dataset PATH --method METHOD --length INT --k INT --pairs INT "
         "[--timing-mode batch|diff] [--n1 INT] [--n2 INT] "
         "[--warmup-passes INT] [--seed INT]\n";
}

Options parse_options(int argc, char** argv) {
  Options opts;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      usage(std::cout);
      std::exit(0);
    }
    if (arg.rfind("--", 0) != 0 || i + 1 >= argc) {
      fail("option requires a value: " + arg);
    }
    const std::string value = argv[++i];
    if (arg == "--dataset") {
      opts.dataset = value;
    } else if (arg == "--method") {
      opts.method = value;
    } else if (arg == "--timing-mode") {
      opts.timing_mode = value;
    } else if (arg == "--length") {
      opts.length = static_cast<int>(parse_signed(value, arg));
    } else if (arg == "--k") {
      opts.k = static_cast<int>(parse_signed(value, arg));
    } else if (arg == "--pairs") {
      opts.pairs = static_cast<size_t>(parse_signed(value, arg));
    } else if (arg == "--n1") {
      opts.n1 = static_cast<size_t>(parse_signed(value, arg));
    } else if (arg == "--n2") {
      opts.n2 = static_cast<size_t>(parse_signed(value, arg));
    } else if (arg == "--warmup-passes") {
      opts.warmup_passes = static_cast<int>(parse_signed(value, arg));
    } else if (arg == "--seed") {
      opts.seed = static_cast<uint64_t>(parse_signed(value, arg));
    } else {
      fail("unknown option: " + arg);
    }
  }
  if (opts.dataset.empty() || opts.method.empty() || opts.length <= 0 ||
      opts.k < 0 || opts.pairs == 0 || opts.warmup_passes < 0) {
    usage(std::cerr);
    fail("missing or invalid required option");
  }
  if (opts.timing_mode != "batch" && opts.timing_mode != "diff") {
    fail("--timing-mode must be batch or diff");
  }
  if (opts.timing_mode == "diff" && (opts.n1 == 0 || opts.n2 <= opts.n1)) {
    fail("diff mode requires 0 < --n1 < --n2");
  }
  return opts;
}

std::vector<std::string> split_tsv(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  for (char c : line) {
    if (c == '\t') {
      fields.push_back(field);
      field.clear();
    } else {
      field.push_back(c);
    }
  }
  fields.push_back(field);
  return fields;
}

std::vector<Pair> read_dataset(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    fail("cannot open dataset: " + path);
  }
  std::string line;
  if (!std::getline(input, line)) {
    fail("empty dataset: " + path);
  }
  const std::vector<std::string> header = split_tsv(line);
  std::unordered_map<std::string, size_t> column;
  for (size_t i = 0; i < header.size(); ++i) {
    column[header[i]] = i;
  }
  for (const std::string& required : {"pair_id", "length", "true_ed", "read", "reference"}) {
    if (!column.count(required)) {
      fail("dataset missing column: " + required);
    }
  }
  std::vector<Pair> pairs;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    const std::vector<std::string> fields = split_tsv(line);
    if (fields.size() != header.size()) {
      fail("invalid TSV row width in " + path);
    }
    Pair pair;
    pair.id = fields[column["pair_id"]];
    pair.length = static_cast<int>(parse_signed(fields[column["length"]], "length"));
    pair.true_ed = static_cast<int>(parse_signed(fields[column["true_ed"]], "true_ed"));
    pair.read = fields[column["read"]];
    pair.reference = fields[column["reference"]];
    if (pair.read.size() != pair.reference.size() ||
        pair.read.size() != static_cast<size_t>(pair.length)) {
      fail("dataset row " + pair.id + " must have equal requested sequence lengths");
    }
    pairs.push_back(std::move(pair));
  }
  if (pairs.empty()) {
    fail("dataset has no pairs: " + path);
  }
  return pairs;
}

void add_result_to_sink(bool pass, int score) {
  g_pass_sink += pass ? 1 : 0;
  g_score_sink += static_cast<uint64_t>(std::max(0, score));
}

void run_lv89_one(const Pair& pair, int k) {
  int32_t score = 0;
  int32_t target_end = 0;
  int32_t query_end = 0;
  lv_ed_basic(
      pair.reference.size(), pair.reference.data(),
      pair.read.size(), pair.read.data(),
      0, -1, &score, &target_end, &query_end, nullptr);
  add_result_to_sink(score <= k, score);
}

mwf_opt_t miniwfa_edit_options() {
  mwf_opt_t options;
  mwf_opt_init(&options);
  options.flag &= ~MWF_F_CIGAR;
  options.x = 1;
  options.o1 = 0;
  options.e1 = 1;
  options.o2 = 0;
  options.e2 = 1;
  options.step = 0;
  options.max_s = 0;
  return options;
}

void run_miniwfa_one(const Pair& pair, int k, const mwf_opt_t& options) {
  mwf_rst_t result = {};
  mwf_wfa_exact(
      nullptr, &options,
      pair.reference.size(), pair.reference.data(),
      pair.read.size(), pair.read.data(), &result);
  add_result_to_sink(result.s <= k, result.s);
  std::free(result.cigar);
}

wavefront_aligner_attr_t wfa2_edit_attributes() {
  wavefront_aligner_attr_t attributes = wavefront_aligner_attr_default;
  attributes.distance_metric = edit;
  attributes.alignment_scope = compute_score;
  attributes.heuristic.strategy = wf_heuristic_none;
  return attributes;
}

int wfa2_score(wavefront_aligner_t* aligner, const Pair& pair) {
  const int status = wavefront_align(
      aligner, pair.read.data(), pair.read.size(),
      pair.reference.data(), pair.reference.size());
  if (status != WF_STATUS_ALG_COMPLETED) {
    fail("WFA2 alignment failed with status " + std::to_string(status));
  }
  return aligner->cigar->score;
}

Measurement measure_non_leap(const Options& opts, const std::vector<Pair>& data, size_t calls) {
  Measurement measurement;
  const mwf_opt_t mini_options = miniwfa_edit_options();
  const wavefront_aligner_attr_t wfa_attributes = wfa2_edit_attributes();
  std::unique_ptr<wavefront_aligner_t, decltype(&wavefront_aligner_delete)> reuse_aligner(
      nullptr, wavefront_aligner_delete);
  if (opts.method == "wfa2_reuse") {
    reuse_aligner.reset(wavefront_aligner_new(const_cast<wavefront_aligner_attr_t*>(&wfa_attributes)));
  }

  auto one = [&](size_t index) {
    const Pair& pair = data[index % data.size()];
    if (opts.method == "lv89") {
      run_lv89_one(pair, opts.k);
    } else if (opts.method == "miniwfa") {
      run_miniwfa_one(pair, opts.k, mini_options);
    } else if (opts.method == "wfa2_reuse") {
      const int score = wfa2_score(reuse_aligner.get(), pair);
      add_result_to_sink(score <= opts.k, score);
    } else if (opts.method == "wfa2_fresh") {
      std::unique_ptr<wavefront_aligner_t, decltype(&wavefront_aligner_delete)> aligner(
          wavefront_aligner_new(const_cast<wavefront_aligner_attr_t*>(&wfa_attributes)),
          wavefront_aligner_delete);
      const int score = wfa2_score(aligner.get(), pair);
      add_result_to_sink(score <= opts.k, score);
    } else {
      fail("unknown non-LEAP method: " + opts.method);
    }
  };

  for (int pass = 0; pass < opts.warmup_passes; ++pass) {
    for (size_t i = 0; i < calls; ++i) {
      one(i);
    }
  }
  const uint64_t pass_before = g_pass_sink;
  const uint64_t score_before = g_score_sink;
  const uint64_t begin_ns = thread_cpu_time_ns();
  for (size_t i = 0; i < calls; ++i) {
    one(i);
  }
  measurement.time_ns = thread_cpu_time_ns() - begin_ns;
  measurement.pass_sink = g_pass_sink - pass_before;
  measurement.score_sink = g_score_sink - score_before;
  return measurement;
}

std::string executable_dir() {
  std::vector<char> path(4096);
  const ssize_t length = readlink("/proc/self/exe", path.data(), path.size() - 1);
  if (length <= 0) {
    fail("cannot locate bench_methods executable for LEAP backend");
  }
  path[length] = '\0';
  std::string full(path.data());
  const size_t slash = full.find_last_of('/');
  return slash == std::string::npos ? "." : full.substr(0, slash);
}

bool cpu_has(const char* feature) {
#if defined(__x86_64__) || defined(__i386__)
  if (std::strcmp(feature, "avx2") == 0) {
    return __builtin_cpu_supports("avx2");
  }
  if (std::strcmp(feature, "avx512f") == 0) {
    return __builtin_cpu_supports("avx512f");
  }
#endif
  return false;
}

Measurement measure_leap(const Options& opts, const std::vector<Pair>& data, size_t calls) {
  const bool avx512 = opts.method == "leap_avx512";
  if ((!avx512 && !cpu_has("avx2")) || (avx512 && !cpu_has("avx512f"))) {
    fail(opts.method + " requested but the running CPU does not expose its ISA");
  }
  const std::string library = executable_dir() + "/" +
      (avx512 ? "libleap_backend_avx512.so" : "libleap_backend_avx2.so");
  void* handle = dlopen(library.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (handle == nullptr) {
    fail("cannot load " + library + ": " + dlerror());
  }
  auto close_backend = +[](void* value) {
    if (value != nullptr) {
      dlclose(value);
    }
  };
  auto close_handle = std::unique_ptr<void, void (*)(void*)>(handle, close_backend);
  dlerror();
  auto measure = reinterpret_cast<leap_backend_measure_fn>(
      dlsym(handle, "leap_backend_measure"));
  if (const char* error = dlerror()) {
    fail("cannot resolve LEAP backend entry point: " + std::string(error));
  }

  std::vector<const char*> reads;
  std::vector<const char*> references;
  std::vector<size_t> lengths;
  reads.reserve(data.size());
  references.reserve(data.size());
  lengths.reserve(data.size());
  for (const Pair& pair : data) {
    reads.push_back(pair.read.c_str());
    references.push_back(pair.reference.c_str());
    lengths.push_back(pair.read.size());
  }
  leap_backend_result_t backend = {};
  if (measure(
          reads.data(), references.data(), lengths.data(), data.size(), calls,
          opts.k, opts.warmup_passes, &backend) != 0 ||
      backend.status != 0) {
    fail("LEAP backend failed: " + std::string(backend.message));
  }
  g_pass_sink += backend.pass_sink;
  g_score_sink += backend.score_sink;
  Measurement measurement;
  measurement.time_ns = backend.time_ns;
  measurement.pass_sink = backend.pass_sink;
  measurement.score_sink = backend.score_sink;
  measurement.timing_batches = backend.timing_batches;
  measurement.notes = backend.message;
  return measurement;
}

bool is_leap(const std::string& method) {
  return method == "leap_avx2" || method == "leap_avx512";
}

Measurement measure_method(const Options& opts, const std::vector<Pair>& data, size_t calls) {
  return is_leap(opts.method) ? measure_leap(opts, data, calls) : measure_non_leap(opts, data, calls);
}

std::string tsv_clean(std::string text) {
  for (char& c : text) {
    if (c == '\t' || c == '\n' || c == '\r') {
      c = ' ';
    }
  }
  return text;
}

void print_header() {
  std::cout
      << "method\tlength\tk\tpair_count\ttiming_mode\tn1\tn2\ttime_n1_ns\ttime_n2_ns"
         "\ttotal_time_ns\tavg_ns_per_pair_batch\tavg_ns_per_pair_diff\twarmup_passes"
         "\tmeasured_boundary\tincluded\texcluded\tresult_sink\tscore_sink\tseed\tnotes\n";
}

void print_row(
    const Options& opts,
    const Measurement& n1,
    const Measurement& n2,
    const Measurement& direct) {
  const bool diff = opts.timing_mode == "diff";
  const double batch_average =
      diff ? 0.0 : static_cast<double>(direct.time_ns) / static_cast<double>(opts.pairs);
  const double diff_average =
      diff ? (static_cast<double>(n2.time_ns) - static_cast<double>(n1.time_ns)) /
                 static_cast<double>(opts.n2 - opts.n1)
           : 0.0;
  const uint64_t pass_sink = diff ? n2.pass_sink : direct.pass_sink;
  const uint64_t score_sink = diff ? n2.score_sink : direct.score_sink;
  const std::string boundary = is_leap(opts.method) ? "run_only" : "full_workflow";
  const std::string included = is_leap(opts.method)
      ? "ed.run forward traversal over prepared chunk objects"
      : "method call; score/pass extraction; threshold comparison; required public workflow cleanup; sink";
  const std::string excluded = is_leap(opts.method)
      ? "construction; init; load_reads; bit conversion; masks; reset; check_pass; result extraction; I/O"
      : "dataset generation; ground truth; TSV parsing; output; report; Python orchestration; build";
  std::string notes = is_leap(opts.method)
      ? "LEAP pass-derived sinks updated outside run-only timing"
      : (opts.method == "wfa2_fresh"
             ? "WFA2 aligner construction and destruction included per pair"
             : "");
  const std::string measurement_note = diff ? n2.notes : direct.notes;
  if (!measurement_note.empty()) {
    notes += notes.empty() ? measurement_note : "; " + measurement_note;
  }

  std::cout << opts.method << '\t' << opts.length << '\t' << opts.k << '\t' << opts.pairs
            << '\t' << opts.timing_mode << '\t' << opts.n1 << '\t' << opts.n2 << '\t'
            << n1.time_ns << '\t' << n2.time_ns << '\t'
            << (diff ? n2.time_ns : direct.time_ns) << '\t'
            << std::fixed << std::setprecision(4) << batch_average << '\t'
            << diff_average << '\t' << opts.warmup_passes << '\t' << boundary << '\t'
            << tsv_clean(included) << '\t' << tsv_clean(excluded) << '\t'
            << pass_sink << '\t' << score_sink << '\t' << opts.seed << '\t'
            << tsv_clean(notes) << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Options opts = parse_options(argc, argv);
    const std::vector<Pair> data = read_dataset(opts.dataset);
    Measurement n1;
    Measurement n2;
    Measurement direct;
    if (opts.timing_mode == "diff") {
      n1 = measure_method(opts, data, opts.n1);
      n2 = measure_method(opts, data, opts.n2);
    } else {
      direct = measure_method(opts, data, opts.pairs);
    }
    print_header();
    print_row(opts, n1, n2, direct);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "bench_methods: " << error.what() << '\n';
    return 2;
  }
}


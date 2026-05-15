#include "bench/affine_oracle.hpp"
#include "bench/aligner_api.hpp"
#include "bench/checksum.hpp"
#include "bench/dataset.hpp"
#include "bench/timer.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

struct Args {
  std::string method;
  bench::Mode mode = bench::Mode::Levenshtein;
  int threshold = 0;
  int mismatch = 2;
  int gap_open = 3;
  int gap_extend = 1;
  std::string input;
  std::string metadata;
  int repeat = 5;
  int warmup = 1;
  std::string output;
};

void usage() {
  std::cerr
      << "bench_align --method METHOD --mode levenshtein|affine --threshold N "
      << "--input pairs.tsv --metadata metadata.tsv --output raw.tsv [--repeat 5] [--warmup 1]\n";
}

Args parse_args(int argc, char** argv) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    const std::string key = argv[i];
    auto need = [&](const char* name) -> std::string {
      if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + name);
      return argv[++i];
    };
    if (key == "--method") args.method = need("--method");
    else if (key == "--mode") args.mode = bench::parse_mode(need("--mode"));
    else if (key == "--threshold") args.threshold = std::stoi(need("--threshold"));
    else if (key == "--mismatch") args.mismatch = std::stoi(need("--mismatch"));
    else if (key == "--gap-open") args.gap_open = std::stoi(need("--gap-open"));
    else if (key == "--gap-extend") args.gap_extend = std::stoi(need("--gap-extend"));
    else if (key == "--input") args.input = need("--input");
    else if (key == "--metadata") args.metadata = need("--metadata");
    else if (key == "--repeat") args.repeat = std::stoi(need("--repeat"));
    else if (key == "--warmup") args.warmup = std::stoi(need("--warmup"));
    else if (key == "--output") args.output = need("--output");
    else if (key == "--help" || key == "-h") {
      usage();
      std::exit(0);
    } else {
      throw std::runtime_error("unknown argument: " + key);
    }
  }
  if (args.method.empty() || args.input.empty() || args.metadata.empty() || args.output.empty()) {
    usage();
    throw std::runtime_error("missing required arguments");
  }
  return args;
}

int truth_score(const bench::PairRecord& pair, const bench::MetadataRecord& meta, const bench::AlignParams& params) {
  if (params.mode == bench::Mode::Levenshtein) {
    return meta.dp_levenshtein_distance >= 0 ? meta.dp_levenshtein_distance : meta.simulated_levenshtein_distance;
  }
  return bench::affine_gap_score(pair.reference, pair.read, params.mismatch, params.gap_open, params.gap_extend);
}

std::string join_notes(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (b.empty()) return a;
  if (a.find(b) != std::string::npos) return a;
  return a + ";" + b;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const Args args = parse_args(argc, argv);
    bench::Dataset dataset = bench::load_dataset(args.input, args.metadata);
    const std::string dataset_name = bench::infer_dataset_name(args.metadata, dataset);
    bool dataset_equal_length = true;
    for (const auto& pair : dataset.pairs) {
      if (pair.reference.size() != pair.read.size()) {
        dataset_equal_length = false;
        break;
      }
    }
    std::unique_ptr<bench::Aligner> aligner = bench::make_aligner(args.method);
    if (!aligner) throw std::runtime_error("unknown method: " + args.method);

    bench::AlignParams params;
    params.mode = args.mode;
    params.threshold = args.threshold;
    params.mismatch = args.mismatch;
    params.gap_open = args.gap_open;
    params.gap_extend = args.gap_extend;

    std::ofstream out(args.output);
    if (!out) throw std::runtime_error("failed to open output: " + args.output);
    out << "method\tmode\tdataset\tthreshold\tmismatch\tgap_open\tgap_extend\trepeat\twarmup\t"
        << "num_pairs\tseconds\tpairs_per_second\tns_per_pair\tchecksum\t"
        << "score_mismatches\tpass_mismatches\tcigar_replay_failures\tnotes\n";

    auto run_once = [&](bool measured) {
      bench::Checksum checksum;
      int score_mismatches = 0;
      int pass_mismatches = 0;
      int cigar_replay_failures = 0;
      std::string notes = aligner->notes();
      for (std::size_t i = 0; i < dataset.pairs.size(); ++i) {
        bench::AlignResult result;
        if (!aligner->supports(params.mode)) {
          result.supported = false;
          result.score_valid = false;
          result.pass = false;
          result.notes = "unsupported_mode";
        } else {
          result = aligner->align(dataset.pairs[i].reference, dataset.pairs[i].read, params);
        }
        const int truth = truth_score(dataset.pairs[i], dataset.metadata[i], params);
        const bool truth_pass = truth <= params.threshold;
        if (result.supported && result.pass != truth_pass) ++pass_mismatches;
        if (result.supported && result.score_valid && result.score != truth) ++score_mismatches;
        if (result.cigar_valid && !result.cigar_replay_ok) ++cigar_replay_failures;
        notes = join_notes(notes, result.notes);
        checksum.add_result(result.score, result.score_valid, result.pass, dataset.pairs[i].read.size());
      }
      return std::tuple<bench::Checksum, int, int, int, std::string>(
          checksum, score_mismatches, pass_mismatches, cigar_replay_failures, notes);
    };

    for (int w = 0; w < args.warmup; ++w) {
      (void)run_once(false);
    }
    for (int r = 0; r < args.repeat; ++r) {
      bench::Timer timer;
      timer.start();
      auto [checksum, score_mismatches, pass_mismatches, cigar_replay_failures, notes] = run_once(true);
      const double seconds = timer.seconds();
      const double pairs_per_second = dataset.pairs.empty() ? 0.0 : dataset.pairs.size() / seconds;
      const double ns_per_pair = dataset.pairs.empty() ? 0.0 : seconds * 1e9 / dataset.pairs.size();
      notes = join_notes(notes, std::string("dataset_equal_length=") + (dataset_equal_length ? "true" : "false"));
      if (aligner->name() == "leap") {
        notes = join_notes(notes, std::string("fallback_used=") +
                                      (notes.find("oracle_fallback") == std::string::npos ? "false" : "true"));
      }
      out << aligner->name() << '\t' << bench::mode_name(params.mode) << '\t' << dataset_name << '\t'
          << params.threshold << '\t' << params.mismatch << '\t' << params.gap_open << '\t'
          << params.gap_extend << '\t' << r << '\t' << args.warmup << '\t'
          << dataset.pairs.size() << '\t' << seconds << '\t' << pairs_per_second << '\t'
          << ns_per_pair << '\t' << checksum.hex() << '\t' << score_mismatches << '\t'
          << pass_mismatches << '\t' << cigar_replay_failures << '\t' << notes << '\n';
    }
  } catch (const std::exception& e) {
    std::cerr << "bench_align: " << e.what() << '\n';
    return 1;
  }
  return 0;
}

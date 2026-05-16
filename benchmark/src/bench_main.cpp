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
  std::string phase_output;
};

struct RunStats {
  bench::Checksum checksum;
  int score_mismatches = 0;
  int pass_mismatches = 0;
  int cigar_replay_failures = 0;
  std::uint64_t total_preprocess_ns = 0;
  std::uint64_t total_forward_ns = 0;
  std::uint64_t total_traceback_ns = 0;
  std::uint64_t total_ns = 0;
  std::string notes;
};

struct CigarReplay {
  bool ok = false;
  int score = 0;
};

void usage() {
  std::cerr
      << "bench_align --method METHOD --mode levenshtein|levenshtein_exact|affine "
      << "--input pairs.tsv --metadata metadata.tsv --output raw.tsv [--threshold N] "
      << "[--phase-output phases.tsv] [--repeat 5] [--warmup 1]\n";
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
    else if (key == "--phase-output") args.phase_output = need("--phase-output");
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
  if (params.mode == bench::Mode::LevenshteinExact) {
    if (meta.dp_levenshtein_distance < 0) {
      throw std::runtime_error("missing dp_levenshtein_distance for exact benchmark pair_id=" + pair.pair_id);
    }
    return meta.dp_levenshtein_distance;
  }
  if (params.mode == bench::Mode::Levenshtein) {
    return meta.dp_levenshtein_distance >= 0 ? meta.dp_levenshtein_distance
                                             : meta.simulated_levenshtein_distance;
  }
  return bench::affine_gap_score(pair.reference, pair.read, params.mismatch, params.gap_open, params.gap_extend);
}

CigarReplay replay_cigar(const std::string& cigar, const std::string& reference, const std::string& read) {
  CigarReplay replay;
  std::size_t rpos = 0;
  std::size_t qpos = 0;
  std::size_t i = 0;
  while (i < cigar.size()) {
    if (cigar[i] < '0' || cigar[i] > '9') return replay;
    std::size_t count = 0;
    while (i < cigar.size() && cigar[i] >= '0' && cigar[i] <= '9') {
      count = count * 10 + static_cast<std::size_t>(cigar[i] - '0');
      ++i;
    }
    if (count == 0 || i >= cigar.size()) return replay;
    const char op = cigar[i++];
    switch (op) {
      case 'M':
        if (qpos + count > read.size() || rpos + count > reference.size()) return replay;
        for (std::size_t j = 0; j < count; ++j) {
          if (read[qpos + j] != reference[rpos + j]) ++replay.score;
        }
        qpos += count;
        rpos += count;
        break;
      case 'I':
        if (qpos + count > read.size()) return replay;
        replay.score += static_cast<int>(count);
        qpos += count;
        break;
      case 'D':
        if (rpos + count > reference.size()) return replay;
        replay.score += static_cast<int>(count);
        rpos += count;
        break;
      default:
        return replay;
    }
  }
  replay.ok = (qpos == read.size() && rpos == reference.size());
  return replay;
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
    const bool exact_mode = params.mode == bench::Mode::LevenshteinExact;
    if (exact_mode) {
      out << "method\tmode\tdataset\trepeat\tnum_pairs\tseconds\ttotal_ns\tavg_total_ns_per_pair\t"
          << "total_preprocess_ns\ttotal_forward_ns\ttotal_traceback_ns\t"
          << "avg_preprocess_ns_per_pair\tavg_forward_ns_per_pair\tavg_traceback_ns_per_pair\t"
          << "preprocess_percent\tforward_percent\ttraceback_percent\t"
          << "score_mismatches\tcigar_replay_failures\tnotes\n";
    } else {
      out << "method\tmode\tdataset\tthreshold\tmismatch\tgap_open\tgap_extend\trepeat\twarmup\t"
          << "num_pairs\tseconds\tpairs_per_second\tns_per_pair\tchecksum\t"
          << "score_mismatches\tpass_mismatches\tcigar_replay_failures\tnotes\n";
    }

    auto run_once = [&]() {
      RunStats stats;
      stats.notes = aligner->notes();
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
        if (exact_mode) {
          bool cigar_ok = false;
          if (result.cigar_valid) {
            const CigarReplay replay = replay_cigar(result.cigar, dataset.pairs[i].reference, dataset.pairs[i].read);
            cigar_ok = replay.ok && replay.score == result.score && replay.score == truth;
          }
          if (!result.score_valid || result.score != truth) ++stats.score_mismatches;
          if (!cigar_ok) ++stats.cigar_replay_failures;
          stats.total_preprocess_ns += result.preprocess_ns;
          stats.total_forward_ns += result.forward_ns;
          stats.total_traceback_ns += result.traceback_ns;
          stats.total_ns += result.total_ns;
        } else {
          const bool truth_pass = truth <= params.threshold;
          if (result.supported && result.pass != truth_pass) ++stats.pass_mismatches;
          if (result.supported && result.score_valid && result.score != truth) ++stats.score_mismatches;
          if (result.cigar_valid && !result.cigar_replay_ok) ++stats.cigar_replay_failures;
          stats.checksum.add_result(result.score, result.score_valid, result.pass, dataset.pairs[i].read.size());
        }
        stats.notes = join_notes(stats.notes, result.notes);
      }
      return stats;
    };

    for (int w = 0; w < args.warmup; ++w) {
      (void)run_once();
    }
    RunStats phase_totals;
    for (int r = 0; r < args.repeat; ++r) {
      bench::Timer timer;
      timer.start();
      RunStats stats = run_once();
      const double seconds = timer.seconds();
      const double pairs_per_second = dataset.pairs.empty() ? 0.0 : dataset.pairs.size() / seconds;
      const double ns_per_pair = dataset.pairs.empty() ? 0.0 : seconds * 1e9 / dataset.pairs.size();
      stats.notes = join_notes(stats.notes, std::string("dataset_equal_length=") + (dataset_equal_length ? "true" : "false"));
      if (aligner->name() == "leap") {
        stats.notes = join_notes(stats.notes, std::string("fallback_used=") +
                                      (stats.notes.find("oracle_fallback") == std::string::npos ? "false" : "true"));
      }
      if (exact_mode) {
        const double avg_total = dataset.pairs.empty() ? 0.0
                                : static_cast<double>(stats.total_ns) / dataset.pairs.size();
        const double avg_pre = dataset.pairs.empty() ? 0.0
                              : static_cast<double>(stats.total_preprocess_ns) / dataset.pairs.size();
        const double avg_fwd = dataset.pairs.empty() ? 0.0
                              : static_cast<double>(stats.total_forward_ns) / dataset.pairs.size();
        const double avg_tb = dataset.pairs.empty() ? 0.0
                             : static_cast<double>(stats.total_traceback_ns) / dataset.pairs.size();
        const double pre_pct = stats.total_ns == 0 ? 0.0
                               : 100.0 * static_cast<double>(stats.total_preprocess_ns) / stats.total_ns;
        const double fwd_pct = stats.total_ns == 0 ? 0.0
                               : 100.0 * static_cast<double>(stats.total_forward_ns) / stats.total_ns;
        const double tb_pct = stats.total_ns == 0 ? 0.0
                              : 100.0 * static_cast<double>(stats.total_traceback_ns) / stats.total_ns;
        out << aligner->name() << '\t' << bench::mode_name(params.mode) << '\t' << dataset_name << '\t'
            << r << '\t' << dataset.pairs.size() << '\t' << seconds << '\t'
            << stats.total_ns << '\t' << avg_total << '\t'
            << stats.total_preprocess_ns << '\t' << stats.total_forward_ns << '\t'
            << stats.total_traceback_ns << '\t' << avg_pre << '\t' << avg_fwd << '\t' << avg_tb << '\t'
            << pre_pct << '\t' << fwd_pct << '\t' << tb_pct << '\t'
            << stats.score_mismatches << '\t' << stats.cigar_replay_failures << '\t' << stats.notes << '\n';
        phase_totals.total_preprocess_ns += stats.total_preprocess_ns;
        phase_totals.total_forward_ns += stats.total_forward_ns;
        phase_totals.total_traceback_ns += stats.total_traceback_ns;
        phase_totals.total_ns += stats.total_ns;
        phase_totals.score_mismatches += stats.score_mismatches;
        phase_totals.cigar_replay_failures += stats.cigar_replay_failures;
        phase_totals.notes = join_notes(phase_totals.notes, stats.notes);
      } else {
        out << aligner->name() << '\t' << bench::mode_name(params.mode) << '\t' << dataset_name << '\t'
            << params.threshold << '\t' << params.mismatch << '\t' << params.gap_open << '\t'
            << params.gap_extend << '\t' << r << '\t' << args.warmup << '\t'
            << dataset.pairs.size() << '\t' << seconds << '\t' << pairs_per_second << '\t'
            << ns_per_pair << '\t' << stats.checksum.hex() << '\t' << stats.score_mismatches << '\t'
            << stats.pass_mismatches << '\t' << stats.cigar_replay_failures << '\t' << stats.notes << '\n';
      }
    }
    if (exact_mode && !args.phase_output.empty()) {
      std::ofstream phase(args.phase_output);
      if (!phase) throw std::runtime_error("failed to open phase output: " + args.phase_output);
      const std::size_t total_pairs = dataset.pairs.size() * static_cast<std::size_t>(args.repeat);
      const double avg_pre = total_pairs == 0 ? 0.0
                            : static_cast<double>(phase_totals.total_preprocess_ns) / total_pairs;
      const double avg_fwd = total_pairs == 0 ? 0.0
                            : static_cast<double>(phase_totals.total_forward_ns) / total_pairs;
      const double avg_tb = total_pairs == 0 ? 0.0
                           : static_cast<double>(phase_totals.total_traceback_ns) / total_pairs;
      const double avg_total = total_pairs == 0 ? 0.0
                              : static_cast<double>(phase_totals.total_ns) / total_pairs;
      const double pre_pct = phase_totals.total_ns == 0 ? 0.0
                             : 100.0 * static_cast<double>(phase_totals.total_preprocess_ns) / phase_totals.total_ns;
      const double fwd_pct = phase_totals.total_ns == 0 ? 0.0
                             : 100.0 * static_cast<double>(phase_totals.total_forward_ns) / phase_totals.total_ns;
      const double tb_pct = phase_totals.total_ns == 0 ? 0.0
                            : 100.0 * static_cast<double>(phase_totals.total_traceback_ns) / phase_totals.total_ns;
      phase << "method\tmode\tdataset\tnum_pairs\ttotal_preprocess_ns\ttotal_forward_ns\t"
            << "total_traceback_ns\ttotal_ns\tavg_preprocess_ns_per_pair\tavg_forward_ns_per_pair\t"
            << "avg_traceback_ns_per_pair\tavg_total_ns_per_pair\tpreprocess_percent\tforward_percent\t"
            << "traceback_percent\tscore_mismatches\tcigar_replay_failures\n";
      phase << aligner->name() << '\t' << bench::mode_name(params.mode) << '\t' << dataset_name << '\t'
            << total_pairs << '\t' << phase_totals.total_preprocess_ns << '\t'
            << phase_totals.total_forward_ns << '\t' << phase_totals.total_traceback_ns << '\t'
            << phase_totals.total_ns << '\t' << avg_pre << '\t' << avg_fwd << '\t' << avg_tb << '\t'
            << avg_total << '\t' << pre_pct << '\t' << fwd_pct << '\t' << tb_pct << '\t'
            << phase_totals.score_mismatches << '\t' << phase_totals.cigar_replay_failures << '\n';
    }
  } catch (const std::exception& e) {
    std::cerr << "bench_align: " << e.what() << '\n';
    return 1;
  }
  return 0;
}

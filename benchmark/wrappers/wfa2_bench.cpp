#include "bench_common.hpp"

#include <chrono>
#include <sstream>

extern "C" {
#include "wavefront/wavefront_align.h"
}

int main(int argc, char** argv) {
  wavefront_aligner_attr_t attributes = wavefront_aligner_attr_default;
  attributes.distance_metric = edit;
  attributes.alignment_scope = compute_score;
  attributes.alignment_form.span = alignment_end2end;
  attributes.heuristic.strategy = wf_heuristic_none;

  try {
    const BenchOptions opt = parse_bench_options(argc, argv, "wfa2", "edit");
    const auto pairs = load_pairs_tsv(opt.input);
    wavefront_aligner_t* wf = wavefront_aligner_new(&attributes);
    uint64_t total_time_ns = 0;
    uint64_t correct_count = 0;
    uint64_t mismatch_count = 0;
    const int max_edit = pairs.empty() ? -1 : pairs.front().max_edit;
    std::vector<std::string> mismatches;

    for (size_t i = 0; i < pairs.size(); ++i) {
      const auto& p = pairs[i];
      if (p.max_edit != max_edit) throw std::runtime_error("mixed max_edit values in dataset");
      if (i != 0) wavefront_aligner_reap(wf);

      const auto t0 = std::chrono::steady_clock::now();
      wavefront_align(wf, p.read.c_str(), static_cast<int>(p.read.size()), p.reference.c_str(),
                      static_cast<int>(p.reference.size()));
      const auto t1 = std::chrono::steady_clock::now();
      total_time_ns += static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

      const int score = wf->cigar->score;
      if (score == p.ground_truth_ed) {
        ++correct_count;
      } else {
        ++mismatch_count;
        std::ostringstream os;
        os << p.pair_id << "\twfa2\tedit\t" << p.read << '\t' << p.reference << '\t'
           << p.ground_truth_ed << '\t' << score;
        mismatches.push_back(os.str());
      }
    }
    wavefront_aligner_delete(wf);

    const uint64_t n = pairs.size();
    const double avg = n ? static_cast<double>(total_time_ns) / static_cast<double>(n) : 0.0;
    std::cout << "method\tbackend\tdataset\tmax_edit\tnum_pairs\ttotal_time_ns\t"
                 "avg_time_ns_per_pair\tcorrect_count\tmismatch_count\n";
    std::cout << "wfa2\tedit\t" << opt.dataset << '\t' << max_edit << '\t' << n << '\t'
              << total_time_ns << '\t' << avg << '\t' << correct_count << '\t'
              << mismatch_count << '\n';
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

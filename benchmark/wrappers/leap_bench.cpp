#include "bench_common.hpp"
#include "SIMD_ED.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
#ifdef USE_AVX512
  const std::string backend = "avx512";
#else
  const std::string backend = "avx2";
#endif

  try {
    int max_energy = 15;
    size_t chunk_size = 0;
    std::vector<char*> forwarded;
    forwarded.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--max-energy" && i + 1 < argc) {
        max_energy = std::stoi(argv[++i]);
      } else if (arg == "--chunk-size" && i + 1 < argc) {
        chunk_size = static_cast<size_t>(std::stoul(argv[++i]));
      } else {
        forwarded.push_back(argv[i]);
      }
    }
    if (chunk_size == 0) chunk_size = LEAP_ACTIVE_EFFECTIVE_LENGTH;
    if (chunk_size > LEAP_ACTIVE_EFFECTIVE_LENGTH) {
      throw std::runtime_error("--chunk-size must be <= LEAP_ACTIVE_EFFECTIVE_LENGTH");
    }
    const BenchOptions opt =
        parse_bench_options(static_cast<int>(forwarded.size()), forwarded.data(), "leap", backend);
    const auto pairs = load_pairs_tsv(opt.input);
    SIMD_ED ed;
    ed.init_levenshtein(max_energy, ED_GLOBAL, false);
    uint64_t total_time_ns = 0;
    uint64_t correct_count = 0;
    uint64_t mismatch_count = 0;
    const int max_edit = pairs.empty() ? -1 : pairs.front().max_edit;
    std::vector<std::string> mismatches;

    for (const auto& p : pairs) {
      if (p.max_edit != max_edit) throw std::runtime_error("mixed max_edit values in dataset");
      if (p.read.size() != p.reference.size()) {
        throw std::runtime_error("LEAP wrapper requires equal-length pairs");
      }
      bool pass = true;
      for (size_t offset = 0; offset < p.read.size(); offset += chunk_size) {
        const size_t chunk_len = std::min(chunk_size, p.read.size() - offset);

        std::array<char, LEAP_ACTIVE_EFFECTIVE_LENGTH> read_chunk{};
        std::array<char, LEAP_ACTIVE_EFFECTIVE_LENGTH> ref_chunk{};
        read_chunk.fill('A');
        ref_chunk.fill('A');
        std::copy_n(p.read.c_str() + offset, chunk_len, read_chunk.data());
        std::copy_n(p.reference.c_str() + offset, chunk_len, ref_chunk.data());

        const auto t0 = std::chrono::steady_clock::now();
        ed.load_reads(read_chunk.data(), ref_chunk.data(), static_cast<int>(chunk_len));
        ed.calculate_masks();
        ed.reset();
        ed.run();
        const auto t1 = std::chrono::steady_clock::now();
        total_time_ns += static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        pass = pass && ed.check_pass();
      }

      const bool expected_pass = p.ground_truth_ed <= max_energy;
      if (pass == expected_pass) {
        ++correct_count;
      } else {
        ++mismatch_count;
        std::ostringstream os;
        os << p.pair_id << "\tleap\t" << backend << '\t' << p.read << '\t' << p.reference
           << '\t' << p.ground_truth_ed << '\t' << (pass ? "pass" : "fail");
        mismatches.push_back(os.str());
      }
    }

    const uint64_t n = pairs.size();
    const double avg = n ? static_cast<double>(total_time_ns) / static_cast<double>(n) : 0.0;
    std::cout << "method\tbackend\tdataset\tmax_edit\tnum_pairs\ttotal_time_ns\t"
                 "avg_time_ns_per_pair\tcorrect_count\tmismatch_count\n";
    std::cout << "leap\t" << backend << '\t' << opt.dataset << '\t' << max_edit << '\t' << n
              << '\t' << total_time_ns << '\t' << avg << '\t' << correct_count << '\t'
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

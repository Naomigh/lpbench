#ifndef BENCH_DATASET_HPP
#define BENCH_DATASET_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace bench {

struct PairRecord {
  std::string pair_id;
  std::string reference;
  std::string read;
};

struct MetadataRecord {
  std::string pair_id;
  std::string dataset_name;
  int ref_length = 0;
  int read_length = 0;
  int bucket = 0;
  int num_substitutions = 0;
  int num_inserted_bases = 0;
  int num_deleted_bases = 0;
  int num_insertion_events = 0;
  int num_deletion_events = 0;
  int simulated_levenshtein_distance = 0;
  int dp_levenshtein_distance = -1;
  int simulated_affine_score = 0;
  std::string edit_script;
  std::uint64_t random_seed = 0;
};

struct Dataset {
  std::vector<PairRecord> pairs;
  std::vector<MetadataRecord> metadata;
};

Dataset load_dataset(const std::string& pairs_path, const std::string& metadata_path);
std::string infer_dataset_name(const std::string& metadata_path, const Dataset& dataset);

}  // namespace bench

#endif


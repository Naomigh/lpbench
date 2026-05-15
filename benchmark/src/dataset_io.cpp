#include "bench/dataset.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace bench {
namespace {

std::vector<std::string> split_tab(const std::string& line) {
  std::vector<std::string> fields;
  std::size_t begin = 0;
  while (true) {
    const std::size_t pos = line.find('\t', begin);
    if (pos == std::string::npos) {
      fields.push_back(line.substr(begin));
      break;
    }
    fields.push_back(line.substr(begin, pos - begin));
    begin = pos + 1;
  }
  return fields;
}

int to_int(const std::string& s, int default_value = 0) {
  if (s.empty()) return default_value;
  return std::stoi(s);
}

std::uint64_t to_u64(const std::string& s) {
  if (s.empty()) return 0;
  return static_cast<std::uint64_t>(std::stoull(s));
}

}  // namespace

Dataset load_dataset(const std::string& pairs_path, const std::string& metadata_path) {
  Dataset dataset;
  std::ifstream pairs_in(pairs_path);
  if (!pairs_in) throw std::runtime_error("failed to open pairs file: " + pairs_path);
  std::string line;
  std::getline(pairs_in, line);
  while (std::getline(pairs_in, line)) {
    if (line.empty()) continue;
    auto fields = split_tab(line);
    if (fields.size() != 3) throw std::runtime_error("bad pairs.tsv row");
    dataset.pairs.push_back({fields[0], fields[1], fields[2]});
  }

  std::ifstream meta_in(metadata_path);
  if (!meta_in) throw std::runtime_error("failed to open metadata file: " + metadata_path);
  std::getline(meta_in, line);
  const auto header = split_tab(line);
  std::unordered_map<std::string, std::size_t> col;
  for (std::size_t i = 0; i < header.size(); ++i) col[header[i]] = i;
  auto get = [&](const std::vector<std::string>& fields, const std::string& name) -> std::string {
    const auto it = col.find(name);
    if (it == col.end() || it->second >= fields.size()) return "";
    return fields[it->second];
  };
  while (std::getline(meta_in, line)) {
    if (line.empty()) continue;
    auto fields = split_tab(line);
    MetadataRecord row;
    row.pair_id = get(fields, "pair_id");
    row.dataset_name = get(fields, "dataset_name");
    row.ref_length = to_int(get(fields, "ref_length"));
    row.read_length = to_int(get(fields, "read_length"));
    row.bucket = to_int(get(fields, "bucket"));
    row.num_substitutions = to_int(get(fields, "num_substitutions"));
    row.num_inserted_bases = to_int(get(fields, "num_inserted_bases"));
    row.num_deleted_bases = to_int(get(fields, "num_deleted_bases"));
    row.num_insertion_events = to_int(get(fields, "num_insertion_events"));
    row.num_deletion_events = to_int(get(fields, "num_deletion_events"));
    row.simulated_levenshtein_distance = to_int(get(fields, "simulated_levenshtein_distance"));
    row.dp_levenshtein_distance = to_int(get(fields, "dp_levenshtein_distance"), -1);
    row.simulated_affine_score = to_int(get(fields, "simulated_affine_score"));
    row.edit_script = get(fields, "edit_script");
    row.random_seed = to_u64(get(fields, "random_seed"));
    dataset.metadata.push_back(std::move(row));
  }
  if (dataset.pairs.size() != dataset.metadata.size()) {
    throw std::runtime_error("pairs and metadata row counts differ");
  }
  for (std::size_t i = 0; i < dataset.pairs.size(); ++i) {
    if (dataset.pairs[i].pair_id != dataset.metadata[i].pair_id) {
      throw std::runtime_error("pair_id mismatch at row " + std::to_string(i));
    }
  }
  return dataset;
}

std::string infer_dataset_name(const std::string&, const Dataset& dataset) {
  if (!dataset.metadata.empty() && !dataset.metadata.front().dataset_name.empty()) {
    return dataset.metadata.front().dataset_name;
  }
  return "unknown";
}

}  // namespace bench


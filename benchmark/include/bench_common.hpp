#pragma once

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace bench {

constexpr int kDefaultLength = 100;
constexpr int kDefaultTrueEdMin = 0;
constexpr int kDefaultTrueEdMax = 15;
constexpr int kDefaultPairsPerEd = 10000;
constexpr int kDefaultSeed = 1;

inline bool starts_with_dash(std::string_view s) {
    return s.size() >= 2 && s[0] == '-' && s[1] == '-';
}

inline int parse_int(const std::string& value, const char* name) {
    char* end = nullptr;
    long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0') {
        throw std::runtime_error(std::string("invalid integer for ") + name + ": " + value);
    }
    return static_cast<int>(parsed);
}

inline std::string default_dataset_path(int length, int ed_min, int ed_max) {
    return "benchmark/data/generated/pairs_len" + std::to_string(length) +
           "_ed" + std::to_string(ed_min) + "_" + std::to_string(ed_max) + ".tsv";
}

inline std::vector<std::pair<int, int>> chunks_for_length(int length, int chunk_size) {
    if (length <= 0) {
        return {};
    }
    std::vector<std::pair<int, int>> chunks;
    for (int offset = 0; offset < length; offset += chunk_size) {
        int n = std::min(chunk_size, length - offset);
        chunks.emplace_back(offset, n);
    }
    return chunks;
}

int trusted_edit_distance(std::string_view a, std::string_view b);
int trusted_edit_distance_banded(std::string_view a, std::string_view b, int max_distance);

}  // namespace bench

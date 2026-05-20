#pragma once

#include "bench_common.hpp"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bench {

struct PairRecord {
    int pair_id = 0;
    int length = 0;
    int true_ed = 0;
    std::string read;
    std::string reference;
    int num_sub = 0;
    int num_ins = 0;
    int num_del = 0;
    std::string edit_pattern;
    int seed = 0;
    std::array<uint8_t, 6> pass_k{};
};

inline uint8_t expected_pass_for_k(const PairRecord& pair, int k) {
    if (k >= 1 && k <= 5) {
        return pair.pass_k[static_cast<std::size_t>(k)];
    }
    return pair.true_ed <= k ? 1 : 0;
}

inline std::vector<std::string> split_tsv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream in(line);
    while (std::getline(in, field, '\t')) {
        fields.push_back(field);
    }
    return fields;
}

inline std::vector<PairRecord> load_dataset_tsv(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open dataset: " + path);
    }

    std::string header;
    if (!std::getline(in, header)) {
        throw std::runtime_error("empty dataset: " + path);
    }

    std::vector<PairRecord> pairs;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        auto fields = split_tsv_line(line);
        if (fields.size() != 15) {
            throw std::runtime_error("bad TSV column count in dataset row: " + line);
        }

        PairRecord p;
        p.pair_id = parse_int(fields[0], "pair_id");
        p.length = parse_int(fields[1], "length");
        p.true_ed = parse_int(fields[2], "true_ed");
        p.read = fields[3];
        p.reference = fields[4];
        p.num_sub = parse_int(fields[5], "num_sub");
        p.num_ins = parse_int(fields[6], "num_ins");
        p.num_del = parse_int(fields[7], "num_del");
        p.edit_pattern = fields[8];
        p.seed = parse_int(fields[9], "seed");
        for (int k = 1; k <= 5; ++k) {
            p.pass_k[static_cast<std::size_t>(k)] =
                static_cast<uint8_t>(parse_int(fields[9 + k], "pass_k"));
        }
        if (static_cast<int>(p.read.size()) != p.length ||
            static_cast<int>(p.reference.size()) != p.length) {
            throw std::runtime_error("dataset row length does not match sequence size");
        }
        pairs.push_back(std::move(p));
    }
    return pairs;
}

inline std::vector<const PairRecord*> select_pairs(
    const std::vector<PairRecord>& pairs, int length, int true_ed_min, int true_ed_max) {
    std::vector<const PairRecord*> selected;
    for (const auto& p : pairs) {
        if (p.length == length && p.true_ed >= true_ed_min && p.true_ed <= true_ed_max) {
            selected.push_back(&p);
        }
    }
    return selected;
}

inline std::vector<const PairRecord*> select_pairs_by_ed(
    const std::vector<PairRecord>& pairs, int length, int true_ed_group) {
    std::vector<const PairRecord*> selected;
    for (const auto& p : pairs) {
        if (p.length == length && p.true_ed == true_ed_group) {
            selected.push_back(&p);
        }
    }
    return selected;
}

}  // namespace bench

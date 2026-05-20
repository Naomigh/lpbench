#include "bench_common.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Args {
    int length = bench::kDefaultLength;
    int true_ed_min = bench::kDefaultTrueEdMin;
    int true_ed_max = bench::kDefaultTrueEdMax;
    int pairs_per_ed = bench::kDefaultPairsPerEd;
    int seed = bench::kDefaultSeed;
    std::string out = bench::default_dataset_path(length, true_ed_min, true_ed_max);
};

struct GeneratedPair {
    std::string read;
    std::string reference;
    int num_sub = 0;
    int num_ins = 0;
    int num_del = 0;
    std::string pattern;
};

Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        auto require_value = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error(std::string("missing value for ") + name);
            }
            return argv[++i];
        };
        if (key == "--length") {
            args.length = bench::parse_int(require_value("--length"), "--length");
        } else if (key == "--true-ed-min") {
            args.true_ed_min = bench::parse_int(require_value("--true-ed-min"), "--true-ed-min");
        } else if (key == "--true-ed-max") {
            args.true_ed_max = bench::parse_int(require_value("--true-ed-max"), "--true-ed-max");
        } else if (key == "--pairs-per-ed") {
            args.pairs_per_ed = bench::parse_int(require_value("--pairs-per-ed"), "--pairs-per-ed");
        } else if (key == "--seed") {
            args.seed = bench::parse_int(require_value("--seed"), "--seed");
        } else if (key == "--out") {
            args.out = require_value("--out");
        } else {
            throw std::runtime_error("unknown argument: " + key);
        }
    }
    if (args.out.empty()) {
        args.out = bench::default_dataset_path(args.length, args.true_ed_min, args.true_ed_max);
    }
    return args;
}

char random_base(std::mt19937_64& rng) {
    static constexpr char bases[] = {'A', 'C', 'G', 'T'};
    std::uniform_int_distribution<int> dist(0, 3);
    return bases[dist(rng)];
}

char different_base(char current, std::mt19937_64& rng) {
    char b = random_base(rng);
    while (b == current) {
        b = random_base(rng);
    }
    return b;
}

std::string random_dna(int length, std::mt19937_64& rng) {
    std::string s;
    s.reserve(static_cast<std::size_t>(length));
    for (int i = 0; i < length; ++i) {
        s.push_back(random_base(rng));
    }
    return s;
}

void apply_substitutions(std::string& read, int count, std::mt19937_64& rng, int& num_sub) {
    if (count <= 0) {
        return;
    }
    std::uniform_int_distribution<int> pos_dist(0, static_cast<int>(read.size()) - 1);
    std::set<int> used;
    while (static_cast<int>(used.size()) < count) {
        used.insert(pos_dist(rng));
    }
    for (int pos : used) {
        read[static_cast<std::size_t>(pos)] = different_base(read[static_cast<std::size_t>(pos)], rng);
        ++num_sub;
    }
}

void apply_balanced_indels(std::string& read, int pairs, std::mt19937_64& rng, int& num_ins, int& num_del) {
    for (int i = 0; i < pairs; ++i) {
        std::uniform_int_distribution<int> del_dist(0, static_cast<int>(read.size()) - 1);
        int del_pos = del_dist(rng);
        read.erase(static_cast<std::size_t>(del_pos), 1);
        ++num_del;

        std::uniform_int_distribution<int> ins_dist(0, static_cast<int>(read.size()));
        int ins_pos = ins_dist(rng);
        read.insert(read.begin() + ins_pos, random_base(rng));
        ++num_ins;
    }
}

GeneratedPair make_candidate(int length, int target_ed, int variant, std::mt19937_64& rng) {
    GeneratedPair p;
    p.reference = random_dna(length, rng);
    p.read = p.reference;

    if (target_ed == 0) {
        p.pattern = "exact";
        return p;
    }

    if (variant == 0 || target_ed == 1) {
        p.pattern = "sub_only";
        apply_substitutions(p.read, target_ed, rng, p.num_sub);
    } else if (variant == 1 && target_ed >= 2) {
        p.pattern = "indel_balanced";
        int indel_pairs = std::max(1, target_ed / 2);
        if (target_ed % 2 != 0) {
            --indel_pairs;
        }
        apply_balanced_indels(p.read, indel_pairs, rng, p.num_ins, p.num_del);
        apply_substitutions(p.read, target_ed - 2 * indel_pairs, rng, p.num_sub);
        if (p.num_sub > 0) {
            p.pattern = "mixed";
        }
    } else {
        p.pattern = "mixed";
        int indel_pairs = std::max(1, target_ed / 3);
        while (2 * indel_pairs >= target_ed && indel_pairs > 0) {
            --indel_pairs;
        }
        apply_substitutions(p.read, target_ed - 2 * indel_pairs, rng, p.num_sub);
        apply_balanced_indels(p.read, indel_pairs, rng, p.num_ins, p.num_del);
    }
    return p;
}

GeneratedPair generate_verified_pair(int length, int target_ed, int pair_index, std::mt19937_64& rng) {
    if (target_ed == 0) {
        auto p = make_candidate(length, target_ed, 0, rng);
        if (bench::trusted_edit_distance(p.read, p.reference) != 0) {
            throw std::runtime_error("internal generator error for exact pair");
        }
        return p;
    }

    for (int attempt = 0; attempt < 20000; ++attempt) {
        int variant = (pair_index + attempt) % 3;
        auto p = make_candidate(length, target_ed, variant, rng);
        if (static_cast<int>(p.read.size()) != length || static_cast<int>(p.reference.size()) != length) {
            continue;
        }
        int ed = bench::trusted_edit_distance(p.read, p.reference);
        if (ed == target_ed) {
            return p;
        }
    }
    throw std::runtime_error("failed to generate verified pair for true_ed=" + std::to_string(target_ed));
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Args args = parse_args(argc, argv);
        if (args.length <= 0 || args.true_ed_min < 0 || args.true_ed_max < args.true_ed_min ||
            args.pairs_per_ed <= 0) {
            throw std::runtime_error("invalid dataset generation arguments");
        }

        std::filesystem::create_directories(std::filesystem::path(args.out).parent_path());
        std::ofstream out(args.out);
        if (!out) {
            throw std::runtime_error("failed to open output dataset: " + args.out);
        }

        out << "pair_id\tlength\ttrue_ed\tread\treference\tnum_sub\tnum_ins\tnum_del\t"
               "edit_pattern\tseed\tpass_k1\tpass_k2\tpass_k3\tpass_k4\tpass_k5\n";

        std::mt19937_64 rng(static_cast<uint64_t>(args.seed));
        int pair_id = 0;
        int total_sub = 0;
        int total_ins = 0;
        int total_del = 0;
        for (int ed = args.true_ed_min; ed <= args.true_ed_max; ++ed) {
            for (int i = 0; i < args.pairs_per_ed; ++i) {
                auto p = generate_verified_pair(args.length, ed, i, rng);
                total_sub += p.num_sub;
                total_ins += p.num_ins;
                total_del += p.num_del;
                out << pair_id++ << '\t'
                    << args.length << '\t'
                    << ed << '\t'
                    << p.read << '\t'
                    << p.reference << '\t'
                    << p.num_sub << '\t'
                    << p.num_ins << '\t'
                    << p.num_del << '\t'
                    << p.pattern << '\t'
                    << args.seed;
                for (int k = 1; k <= 5; ++k) {
                    out << '\t' << (ed <= k ? 1 : 0);
                }
                out << '\n';
            }
        }

        if (args.true_ed_max > 0 && (total_sub == 0 || total_ins == 0 || total_del == 0)) {
            throw std::runtime_error("generated dataset did not include all edit operation types");
        }

        std::cerr << "Generated " << pair_id << " pairs at " << args.out << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "dataset_generate: " << e.what() << "\n";
        return 1;
    }
}

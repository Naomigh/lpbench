#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace bench {

inline void touch_bytes(const std::string& s, volatile uint64_t& sink) {
    for (unsigned char c : s) {
        sink += c;
    }
}

inline void evict_coldish_cache(std::vector<uint8_t>& buffer, volatile uint64_t& sink) {
    constexpr std::size_t stride = 64;
    for (std::size_t i = 0; i < buffer.size(); i += stride) {
        buffer[i] = static_cast<uint8_t>(buffer[i] + 1);
        sink += buffer[i];
    }
}

}  // namespace bench

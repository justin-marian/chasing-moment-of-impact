/**
 * @file
 * Compute stable command keys for the project command-dispatch switches.
 *
 * Provides a case-sensitive FNV-1a hash that converts exact command text
 * into 64-bit keys for switch-based CLI and protocol dispatch.
 */
#pragma once

#include <cstdint>
#include <string_view>


namespace utils {

/**
 * Compute a stable command key for switch-based dispatch.
 *
 * Uses a case-sensitive FNV-1a mapping so that all callers resolve identical
 * command bytes to the same 64-bit identifier.
 *
 * @param text Exact command bytes to hash.
 * @return Stable 64-bit case-sensitive FNV-1a command key.
 */
constexpr std::uint64_t commandKey(std::string_view text) {
    constexpr std::uint64_t OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t PRIME = 1099511628211ULL;

    std::uint64_t result = OFFSET_BASIS;
    for (unsigned char ch : text) {
        result ^= ch;
        result *= PRIME;
    }
    return result;
}

} // namespace utils

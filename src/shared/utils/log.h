/**
 * @file
 * Retain bounded text histories independently of their viewer/world UI display.
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>


namespace utils {

/**
 * Append one event and discard the oldest entry when a retained history
 * exceeds its capacity.
 *
 * @param messages Ordered history owned by the caller.
 * @param text     Event text copied into the history.
 * @param capacity Maximum retained entries, including the new event.
 */
inline void appendLog(std::vector<std::string>& messages, std::string_view text, std::size_t capacity) {
    messages.emplace_back(text);
    if (messages.size() > capacity)
        messages.erase(messages.begin());
}

} // namespace utils

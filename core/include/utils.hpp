#pragma once

#include "datatypes.hpp" // Include datatypes if utils operate on them
#include <string>
#include <chrono>

namespace core {
namespace utils {

    // Example utility: Convert Timestamp to ISO 8601 string
    std::string timestampToString(const Timestamp& ts);

    // Example utility: Parse ISO 8601 string to Timestamp
    Timestamp stringToTimestamp(const std::string& iso_string);

    // Add other common utilities here (e.g., string manipulation, math helpers)

} // namespace utils
} // namespace core
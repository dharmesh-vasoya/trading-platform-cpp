#include "utils.hpp"
#include <iomanip> // For std::put_time, std::get_time
#include <sstream> // For string streams
#include <iomanip>    // For std::get_time
#include <sstream>    // For std::istringstream
#include <string>     // For std::string
#include <stdexcept>  // For std::runtime_error
#include <cmath>      // For std::pow
#include <chrono>     // Ensure chrono is included
#include <cstring>    // For timegm if needed (see below)

namespace core {
namespace utils {

    // Note: Time zone handling can be complex. This example uses system's local time or UTC.
    // Consider using a dedicated date/time library (like Howard Hinnant's date lib or Boost.DateTime)
    // for more robust time zone management if required.
    Timestamp stringToTimestamp(const std::string& iso_string) {
        std::tm tm = {};
        std::istringstream ss(iso_string);
    
        // 1. Parse main date/time part up to seconds
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            throw std::runtime_error("Failed to parse timestamp (date/time part): " + iso_string);
        }
    
        // 2. Manually parse optional fractional seconds
        double fractional_seconds = 0.0;
        if (ss.peek() == '.') {
            ss.ignore(); // consume '.'
            std::string digits;
            int digit_count = 0;
            while (std::isdigit(ss.peek()) && digit_count < 9) { // Limit precision (e.g., nanoseconds)
                digits += static_cast<char>(ss.get());
                digit_count++;
            }
            // Consume any remaining digits after precision limit
             while (std::isdigit(ss.peek())) {
                  ss.ignore();
             }
    
            if (!digits.empty()) {
                try {
                    fractional_seconds = std::stod(digits) / std::pow(10.0, digits.length());
                } catch (const std::exception& e) {
                    throw std::runtime_error(std::string("Failed to parse timestamp (subsecond conversion): ") + e.what());
                }
            }
        }
    
        // 3. Manually parse timezone offset (+HH:MM, -HH:MM, or Z)
        std::chrono::seconds offset_duration = std::chrono::seconds(0);
        char sign_or_z = 0;
    
        if (ss >> sign_or_z) { // Try to read the next character
            if (sign_or_z == 'Z') {
                // UTC
                offset_duration = std::chrono::seconds(0);
            } else if (sign_or_z == '+' || sign_or_z == '-') {
                int offset_h = 0;
                int offset_m = 0;
                char colon = ' ';
                // Read HH:MM after the sign
                if (!(ss >> std::setw(2) >> offset_h >> colon >> std::setw(2) >> offset_m) || colon != ':') {
                     throw std::runtime_error("Failed to parse timestamp (timezone offset HH:MM): " + iso_string);
                }
                offset_duration = std::chrono::hours(offset_h) + std::chrono::minutes(offset_m);
                if (sign_or_z == '-') {
                    offset_duration *= -1; // Make offset negative if sign is '-'
                }
            } else {
                throw std::runtime_error("Invalid timezone indicator '" + std::string(1, sign_or_z) + "' in timestamp: " + iso_string);
            }
        } else {
            // If reading the character failed, maybe the string ended right after seconds/subseconds
            // Treat as UTC? Or local? Or error? Let's error for now, as DB has offsets.
             throw std::runtime_error("Timestamp missing or invalid timezone offset/indicator: " + iso_string);
        }
    
    
        // 4. Convert tm to time_t (UTC seconds since epoch)
        // timegm interprets struct tm as UTC. Required because we apply the offset later.
        // timegm is POSIX standard but not C++ standard. Use _mkgmtime on Windows.
        #ifdef _WIN32
            time_t tt = _mkgmtime(&tm);
        #else
            // Need to declare timegm if not implicitly available (e.g. some C++ standards)
            // extern "C" time_t timegm(struct tm *tm); // Usually in <time.h> on POSIX
            time_t tt = timegm(&tm);
        #endif
        if (tt == (time_t)-1) {
             throw std::runtime_error("Failed to convert parsed date/time to UTC epoch seconds: " + iso_string);
        }
    
        // 5. Create base time_point (UTC) from epoch seconds
        auto base_tp_utc = std::chrono::system_clock::from_time_t(tt);
    
        // 6. Add fractional seconds (as duration)
        base_tp_utc += std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<double>(fractional_seconds));
    
        // 7. Adjust the UTC time_point by the *inverse* of the offset to get the final UTC equivalent time_point.
        // Example: 2015-04-20T00:00:00+05:30
        // - Base UTC from timegm(tm) is 2015-04-20T00:00:00Z
        // - Offset is +5h30m
        // - Final UTC = Base UTC - Offset = 2015-04-19T18:30:00Z
        Timestamp final_tp = base_tp_utc - offset_duration;
    
        return final_tp;
    }
    
    std::string timestampToString(const Timestamp& ts) {
        auto tt = std::chrono::system_clock::to_time_t(ts);
        // Note: system_clock::to_time_t gives UTC epoch seconds.
        // We need to convert this to IST for display matching the DB.
        // Adding the IST offset (5 hours 30 minutes) to the UTC time_point
        // before converting to tm struct via localtime functions.
        // This is one way to handle it without external timezone libs.
        auto ist_time_point = ts + std::chrono::hours(5) + std::chrono::minutes(30);
        auto tt_ist = std::chrono::system_clock::to_time_t(ist_time_point);
    
        // Get milliseconds from the ORIGINAL UTC timepoint 'ts' before offset adjustment
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ts.time_since_epoch()) % 1000;
        if (ms.count() < 0) ms += std::chrono::seconds{1}; // Adjust for negative remainders
    
        std::tm time_tm;
        // Use thread-safe versions if available
        #ifdef _WIN32
            gmtime_s(&time_tm, &tt_ist); // Use gmtime_s/gmtime_r to format the adjusted time parts as if they were UTC figures
        #elif defined(__unix__) || defined(__APPLE__)
            gmtime_r(&tt_ist, &time_tm);
        #else
            std::tm* temp_tm = std::gmtime(&tt_ist);
            if (temp_tm) {
                time_tm = *temp_tm;
            } else {
                throw std::runtime_error("Failed to get gmtime representation for adjusted IST time.");
            }
        #endif
    
        std::ostringstream oss;
        // Format as YYYY-MM-DDTHH:MM:SS using the adjusted time parts
        oss << std::put_time(&time_tm, "%Y-%m-%dT%H:%M:%S");
        // Append milliseconds (Optional - check if your DB strings include them)
        // oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
        // Append the required +05:30 offset string MANDATORILY to match DB format
        // WARNING: This assumes the underlying data *should* be IST.
        oss << "+05:30";
        return oss.str();
    }
    

} // namespace utils
} // namespace core
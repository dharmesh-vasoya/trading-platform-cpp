// In core/src/logging.cpp

// Make sure necessary includes are at the top
#include "logging.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h> // If using async
#include <vector>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <chrono>       // For timestamp in filename
#include <sstream>      // For formatting filename
#include <iomanip>      // For std::put_time
#include <filesystem>   // For creating directory (C++17)
#include <algorithm>    // For std::min, std::transform
#include <cctype>       // For std::tolower

namespace core {
namespace logging {

    static std::shared_ptr<spdlog::logger> global_logger;

    // Corrected initialize function definition
    // Both console_level AND file_level should be spdlog::level::level_enum
    void initialize(const std::string& base_log_filename,
                    spdlog::level::level_enum console_level, // Correct type
                    spdlog::level::level_enum file_level)    // Correct type
    {
        try {
            // --- Check Environment Variable for Override ---
            const char* env_level_cstr = std::getenv("SPDLOG_LEVEL");
            if (env_level_cstr) {
                std::string env_level_str(env_level_cstr);
                spdlog::level::level_enum env_level = level_from_string(env_level_str);
                console_level = env_level;
                file_level = env_level; // This now correctly assigns enum to enum
                std::cout << "[Logging] Overriding log level from SPDLOG_LEVEL environment variable to: "
                          << env_level_str << std::endl;
            }

            // --- Create Log Directory ---
            std::string log_dir = "logs";
            try {
                 if (!std::filesystem::exists(log_dir)) {
                     std::filesystem::create_directories(log_dir);
                     std::cout << "[Logging] Created log directory: " << log_dir << std::endl;
                 }
            } catch (const std::filesystem::filesystem_error& fs_err) {
                std::cerr << "[Logging] Error creating log directory '" << log_dir << "': " << fs_err.what() << std::endl;
                log_dir = "."; // Fallback
            } catch (const std::exception& e) {
                 std::cerr << "[Logging] Error creating log directory '" << log_dir << "': " << e.what() << std::endl;
                 log_dir = "."; // Fallback
            }


            // --- Generate Log Filename with UTC Timestamp ---
            auto now = std::chrono::system_clock::now();
            auto itt = std::chrono::system_clock::to_time_t(now);
            std::tm utc_tm;
            #ifdef _WIN32
                gmtime_s(&utc_tm, &itt);
            #else
                gmtime_r(&itt, &utc_tm);
            #endif

            std::ostringstream filename_oss;
            filename_oss << base_log_filename << "_" << std::put_time(&utc_tm, "%Y%m%d_%H%M%SZ") << ".log";
            std::string log_file_path = log_dir + "/" + filename_oss.str();
            std::cout << "[Logging] Log file path: " << log_file_path << std::endl;

            // --- Define UTC Log Pattern ---
            std::string utc_pattern = "[%Y-%m-%d %H:%M:%S.%e%z] [%^%l%$] [%n] %v";


            // --- Create Sinks ---
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(console_level); // Expects enum
            console_sink->set_pattern(utc_pattern); // Use UTC pattern

            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file_path, 1024 * 1024 * 10, 5, true);
            file_sink->set_level(file_level); // Expects enum - This should now work
            file_sink->set_pattern(utc_pattern); // Use UTC pattern

            // --- Create Logger ---
            std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
            global_logger = std::make_shared<spdlog::logger>("PlatformLogger", sinks.begin(), sinks.end());
            spdlog::register_logger(global_logger);
            spdlog::set_default_logger(global_logger);
            // std::min now works because both arguments are level_enum
            spdlog::set_level(std::min(console_level, file_level));
            spdlog::flush_on(spdlog::level::err);

            // Use getLogger() which checks initialization now
            // to_string_view now works because file_level is the correct type
            #ifdef NDEBUG
                // NDEBUG is defined, so this is likely a Release build
                const char* build_type_str = "Release";
            #else
                // NDEBUG is not defined, so this is likely a Debug build
                const char* build_type_str = "Debug";
            #endif

            // Now use the variable in the log message
            getLogger()->info("Logging initialized (Build Type: {}). Console: {}, File: {} (UTC)",
                            build_type_str, // Use the string variable here
                            spdlog::level::to_string_view(console_level),
                            spdlog::level::to_string_view(file_level));


        } catch (const spdlog::spdlog_ex& ex) {
             std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        } catch (const std::filesystem::filesystem_error& fs_err) {
             std::cerr << "Log initialization failed (Filesystem Error): " << fs_err.what() << std::endl;
        } catch (const std::exception& ex) {
             std::cerr << "Log initialization failed (std::exception): " << ex.what() << std::endl;
        } catch (...) {
             std::cerr << "Log initialization failed (unknown exception)." << std::endl;
        }
    }

    // Keep getLogger() and level_from_string() as they were...
    std::shared_ptr<spdlog::logger>& getLogger() {
        // ...
        if (!global_logger) {
             throw std::runtime_error("Logger accessed before initialization. Call core::logging::initialize() first.");
        }
        return global_logger;
    }

    spdlog::level::level_enum level_from_string(const std::string& level_str) {
        // ...
        std::string lower_str = level_str;
         std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
            [](unsigned char c){ return std::tolower(c); }); // Use locale-safe lower
        // ... (rest of level checks) ...
         if (lower_str == "trace") return spdlog::level::trace;
         if (lower_str == "debug") return spdlog::level::debug;
         if (lower_str == "info") return spdlog::level::info;
         if (lower_str == "warn" || lower_str == "warning") return spdlog::level::warn;
         if (lower_str == "error" || lower_str == "err") return spdlog::level::err;
         if (lower_str == "critical" || lower_str == "crit") return spdlog::level::critical;
         if (lower_str == "off") return spdlog::level::off;
         std::cerr << "[Logging] Unrecognized log level string: '" << level_str << "'. Defaulting to 'info'." << std::endl;
         return spdlog::level::info;
    }

} // namespace logging
} // namespace core
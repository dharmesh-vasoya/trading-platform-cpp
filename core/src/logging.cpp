#include "logging.hpp"
#include <spdlog/sinks/stdout_color_sinks.h> // For console logging
#include <spdlog/sinks/rotating_file_sink.h> // For file logging
#include <spdlog/async.h> // Optional: for async logging
#include <vector>
#include <memory>
#include <iostream> // For error messages during init
#include <cstdlib> // For std::getenv

namespace core {
namespace logging {

    // Global logger instance
    static std::shared_ptr<spdlog::logger> global_logger;

    void initialize(const std::string& log_file_path,
                    spdlog::level::level_enum console_level,
                    spdlog::level::level_enum file_level) {
        try {
            // --- Check Environment Variable for Override ---
            const char* env_level_cstr = std::getenv("SPDLOG_LEVEL");
            if (env_level_cstr) {
                std::string env_level_str(env_level_cstr);
                spdlog::level::level_enum env_level = level_from_string(env_level_str);
                // Override both console and file if env var is set
                console_level = env_level;
                file_level = env_level;
                std::cout << "[Logging] Overriding log level from SPDLOG_LEVEL environment variable to: "
                          << env_level_str << std::endl;
            }


            // --- Create Sinks ---
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(console_level);
            // Example format: [2025-04-20 10:30:00.123] [info] [main] Log message
             console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");


            // Rotating file sink (e.g., 5 files, 10MB each)
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file_path, 1024 * 1024 * 10, 5, true); // path, max_size, max_files, rotate_on_open
            file_sink->set_level(file_level);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] [thread %t] %v");


            // --- Create Logger ---
            // Combine sinks
            std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};

            // Create a logger (can be synchronous or asynchronous)
            // Option 1: Synchronous logger
            global_logger = std::make_shared<spdlog::logger>("PlatformLogger", sinks.begin(), sinks.end());

            // Option 2: Asynchronous logger (more performant but requires linking against std::thread)
            // spdlog::init_thread_pool(8192, 1); // queue size, number of threads
            // global_logger = std::make_shared<spdlog::async_logger>("PlatformLogger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

            // Register the logger so it can be retrieved globally if needed
            spdlog::register_logger(global_logger);

            // Set the default logger instance
            spdlog::set_default_logger(global_logger);

            // Set global log level (most verbose level of all sinks)
            spdlog::set_level(std::min(console_level, file_level));

            // Flush logs on critical errors
            spdlog::flush_on(spdlog::level::err);

            // Optionally remove compile-time logs below a certain level for release builds
            #ifndef NDEBUG // Typically defined in Debug builds
                spdlog::get("PlatformLogger")->info("Logging initialized (Debug Build). Console: {}, File: {}",
                                                    spdlog::level::to_string_view(console_level),
                                                    spdlog::level::to_string_view(file_level));
            #else
                spdlog::get("PlatformLogger")->info("Logging initialized (Release Build). Console: {}, File: {}",
                                                    spdlog::level::to_string_view(console_level),
                                                    spdlog::level::to_string_view(file_level));
            #endif


        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr << "Log initialization failed: " << ex.what() << std::endl;
            // Fallback or exit? For now, just report error.
        } catch (const std::exception& ex) {
             std::cerr << "Log initialization failed (std::exception): " << ex.what() << std::endl;
        } catch (...) {
             std::cerr << "Log initialization failed (unknown exception)." << std::endl;
        }
    }

    std::shared_ptr<spdlog::logger>& getLogger() {
        // Ensure logger is initialized before use
        if (!global_logger) {
             // Initialize with defaults if not called explicitly
             // Or throw an error - throwing might be better to enforce explicit init
            // initialize(); // Avoid silent initialization
             throw std::runtime_error("Logger accessed before initialization. Call core::logging::initialize() first.");
        }
        return global_logger;
    }

    spdlog::level::level_enum level_from_string(const std::string& level_str) {
        std::string lower_str = level_str;
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
        if (lower_str == "trace") return spdlog::level::trace;
        if (lower_str == "debug") return spdlog::level::debug;
        if (lower_str == "info") return spdlog::level::info;
        if (lower_str == "warn" || lower_str == "warning") return spdlog::level::warn;
        if (lower_str == "error" || lower_str == "err") return spdlog::level::err;
        if (lower_str == "critical" || lower_str == "crit") return spdlog::level::critical;
        if (lower_str == "off") return spdlog::level::off;
        // Default level if string is unrecognized
        std::cerr << "[Logging] Unrecognized log level string: '" << level_str << "'. Defaulting to 'info'." << std::endl;
        return spdlog::level::info;
    }

} // namespace logging
} // namespace core
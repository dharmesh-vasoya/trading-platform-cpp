#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <memory>

namespace core {
namespace logging {

    // Call this once at the beginning of your application (e.g., in main())
    void initialize(const std::string& log_file_path = "trading_platform.log",
                    spdlog::level::level_enum console_level = spdlog::level::info,
                    spdlog::level::level_enum file_level = spdlog::level::debug);

    // Get the globally configured logger
    std::shared_ptr<spdlog::logger>& getLogger();

    // Helper function to set log level from string (useful for env vars/args)
    spdlog::level::level_enum level_from_string(const std::string& level_str);

} // namespace logging
} // namespace core
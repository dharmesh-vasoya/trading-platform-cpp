// cli/src/main.cpp

// Standard includes
#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <algorithm>
#include <fstream>     // For std::ifstream

// Project includes (Using short paths)
#include "logging.hpp"
#include "exceptions.hpp"
#include "datatypes.hpp"
#include "utils.hpp"
#include "database_manager.hpp"
#include "strategy_factory.hpp"
#include "backtester.hpp"       // Include Backtester header

// Lib includes
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <nlohmann/json.hpp>
#include <CLI/CLI.hpp>         // CLI11 Include

using json = nlohmann::json; // Alias

int main(int argc, char* argv[]) {
    // Define logger pointer early
    std::shared_ptr<spdlog::logger> logger = nullptr;

    // Initialize Logging Early (Can log basic info and parsing errors)
    // File details (name/location) are determined inside initialize
    core::logging::initialize("trading_platform_cli", spdlog::level::info, spdlog::level::trace);
    logger = core::logging::getLogger();

    // --- Argument Parsing ---
    CLI::App app{"C++ Trading Platform Backtester"};

    // Add version flag (requires TRADING_PLATFORM_VERSION definition in CMakeLists.txt)
    #ifdef TRADING_PLATFORM_VERSION
        app.set_version_flag("--version", std::string(TRADING_PLATFORM_VERSION));
    #else
         app.set_version_flag("--version", std::string("0.1.0")); // Fallback version
    #endif

    // Define argument variables with defaults or to be required
    std::string strategy_file_path; // Required, no default
    std::string start_date;         // Required, no default
    std::string end_date;           // Required, no default
    double initial_capital = 100000.0; // Default capital
    std::string db_path = "/home/vboxuser/market_data_vm_copy.db"; // Default DB Path

    app.add_option("-s,--strategy", strategy_file_path, "Path to the strategy JSON configuration file")
        ->required()
        ->check(CLI::ExistingFile);
    app.add_option("--start", start_date, "Backtest start date (YYYY-MM-DD format)")
        ->required();
    app.add_option("--end", end_date, "Backtest end date (YYYY-MM-DD format)")
        ->required();
    app.add_option("-c,--capital", initial_capital, "Initial capital for the backtest")
        ->check(CLI::PositiveNumber);
    app.add_option("-d,--database", db_path, "Path to the SQLite market data DB file")
        ->check(CLI::ExistingFile);

    // Parse arguments - CLI11 handles --help / -h and errors
    try {
         app.parse(argc, argv);
    } catch (const CLI::ParseError &e) {
         // Use app.exit to print help/error and exit on parse error
         return app.exit(e);
    }
    // --- Argument Parsing Complete ---


    // --- Main Application Logic in a try block ---
    try {
        logger->info("Trading Platform CLI starting..."); // Log now that parse succeeded
        logger->info("Arguments Parsed Successfully:");
        logger->info("  -> Strategy File: {}", strategy_file_path);
        logger->info("  -> Start Date: {}", start_date);
        logger->info("  -> End Date: {}", end_date);
        logger->info("  -> Initial Capital: {:.2f}", initial_capital);
        logger->info("  -> Database Path: {}", db_path);


        // --- Database Setup (Using path from args) ---
        logger->info("Using SQLite database path: {}", db_path);
        data::DatabaseManager db_manager(db_path); // Define db_manager


        // ======================================================
        // --- Run Backtest ---
        // ======================================================
        logger->info("---=== Starting Backtest Run ===---");

        // 1. Load Strategy Config from JSON (path from parsed args)
        json strategy_config;
        logger->info("Loading strategy config from: {}", strategy_file_path);
        try { // Inner try-catch specifically for file I/O and JSON parsing
            std::ifstream ifs(strategy_file_path);
            if (!ifs.is_open()) {
                throw std::runtime_error(fmt::format("Failed to open strategy file: {}", strategy_file_path));
            }
            strategy_config = json::parse(ifs);
            ifs.close();
            logger->info("Strategy config loaded successfully.");
        } catch (const std::exception& e) {
             logger->critical("Failed to load or parse strategy config file '{}': {}", strategy_file_path, e.what());
             // Re-throw or return error code to be caught by outer catch block
             throw; // Re-throw to be caught below
        }

        // 2. Backtest Parameters are already parsed from args
        logger->info("Backtest Parameters: Capital={:.2f}, Start={}, End={}", initial_capital, start_date, end_date);


        // 3. Create and Run Backtester
        backtester::Backtester the_backtester(db_manager, initial_capital); // Use parsed capital
        bool success = the_backtester.run(strategy_config, start_date, end_date); // Use parsed dates

        if (success) {
             logger->info("---=== Backtest Run Finished Successfully ===---");
             // TODO: Print final metrics from backtester result more formally
        } else {
              logger->error("---=== Backtest Run Failed ===---");
              // Consider returning non-zero exit code on backtest failure?
              // return 1; // Optional: Indicate failure
        }
        // ======================================================
        // --- End Backtest Run ---
        // ======================================================

        logger->info("Trading Platform CLI finished.");

    // --- Main Exception Handling ---
    // Catch exceptions from the main application logic (DB, Backtester, etc.)
    } catch (const core::TradingPlatformException& ex) {
        std::cerr << "Platform Error: " << ex.what() << std::endl;
        if(logger) logger->critical("Platform Error: {}", ex.what());
        return 1; // Exit with error code
    } catch (const std::exception& ex) {
        // Catch standard exceptions (including re-thrown ones from inner try)
        std::cerr << "Standard Error: " << ex.what() << std::endl;
        if(logger) logger->critical("Standard Error: {}", ex.what());
        return 1; // Exit with error code
    } catch (...) {
        std::cerr << "Unknown Error occurred." << std::endl;
        if(logger) logger->critical("Unknown Error occurred.");
        return 1; // Exit with error code
    }
    // --- End Main Exception Handling ---


    return 0; // Success exit code

} // End main() - This brace now correctly closes the main function
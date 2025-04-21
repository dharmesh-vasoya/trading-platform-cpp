// cli/src/main.cpp

// Standard includes
#include <iostream>
#include <string>      // Needed for std::string path
#include <vector>      // Needed for TimeSeries/vector
#include <exception>   // Needed for std::exception
#include <chrono>      // Needed for creating test timestamps
#include <cstdlib>     // Needed for std::getenv
#include <memory>      // For std::shared_ptr
#include <algorithm>   // For std::min

// Project includes
#include "logging.hpp"        // For logging functionality
#include "exceptions.hpp"     // For custom exception types
#include "datatypes.hpp"      // For core::Candle, core::Timestamp, etc.
#include "utils.hpp"          // For timestampToString/stringToTimestamp helpers
#include "database_manager.hpp" // Include the DB manager (SQLite based)
#include "upstox_api_client.hpp"// Include the API client
#include "sma_indicator.hpp"// Include the SMA indicator <<<--- ADDED

// Include spdlog header directly for logger type if needed by catch blocks
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

#include <fstream>            // For std::ifstream
#include <nlohmann/json.hpp>  // For json parsing
#include "strategy_factory.hpp" // Include the factory class header
#include "backtester.hpp" // <<<--- ADD THIS
#include <memory>             // For std::unique_ptr

int main(int argc, char* argv[]) {
    // Define logger pointer early in the main scope
    std::shared_ptr<spdlog::logger> logger = nullptr;

    // Main try block for exception handling
    try {
        // --- Initialize Logging ---
        core::logging::initialize("trading_platform_cli.log", spdlog::level::info, spdlog::level::trace); // Use trace for file log during debug
        logger = core::logging::getLogger(); // Assign the initialized logger
        logger->info("Trading Platform CLI starting...");

        // --- Argument Parsing (Placeholder) ---
        if (argc > 1) {
             logger->info("Arguments provided: {}", argc - 1);
        }

        // --- Read API Credentials ---
        const char* api_key_env = std::getenv("UPSTOX_API_KEY");
        const char* api_secret_env = std::getenv("UPSTOX_API_SECRET");
        const char* access_token_env = std::getenv("UPSTOX_ACCESS_TOKEN");
        // TODO: Replace with your actual redirect URI or read from config/env
        const char* redirect_uri_env = "YOUR_CONFIGURED_REDIRECT_URI";

        std::string api_key = api_key_env ? api_key_env : "";
        std::string api_secret = api_secret_env ? api_secret_env : "";
        std::string access_token = access_token_env ? access_token_env : "";
        std::string redirect_uri = redirect_uri_env ? redirect_uri_env : "";

        bool has_api_credentials = true;
        if (api_key.empty() || api_secret.empty()) {
            logger->warn("API Key or Secret not found in environment variables (UPSTOX_API_KEY, UPSTOX_API_SECRET)");
            has_api_credentials = false;
        }
        if (access_token.empty()) {
            logger->warn("Access Token not found in environment variable (UPSTOX_ACCESS_TOKEN). API calls will likely fail.");
            has_api_credentials = false;
        }
         if (redirect_uri.empty() || redirect_uri == "YOUR_CONFIGURED_REDIRECT_URI") {
            logger->warn("Redirect URI is not set or using placeholder.");
         }


        // --- Database Setup ---
        std::string db_path = "/home/vboxuser/market_data_vm_copy.db"; // Path on native VM FS
        logger->info("Using SQLite database path: {}", db_path);
        data::DatabaseManager db_manager(db_path); // Define db_manager here

        // --- Combined DB and API Test Block ---
        logger->info("--- Running Tests ---");
        if (db_manager.connect()) {
            logger->info("DB connection successful.");

            if (db_manager.initializeSchema()) {
                logger->info("DB schema initialization check successful.");

                core::TimeSeries<core::Candle> fetched_candle_data; // To store data for indicator test

                // --- Test queryCandles (DB Read) ---
                logger->info("--- Testing queryCandles ---");
                try {
                    // Fetch a suitable range for the SMA test below
                    std::string test_instrument = "NSE_EQ|INE002A01018"; // Use known good params
                    std::string test_interval = "day";
                    core::Timestamp start_query = core::utils::stringToTimestamp("2016-04-01T00:00:00+05:30"); // Fetch ~3 months for SMA(10) test
                    core::Timestamp end_query   = core::utils::stringToTimestamp("2016-10-30T23:59:59+05:30");

                    logger->info("Querying DB candles for {} ({}) from {} to {}",
                                test_instrument, test_interval,
                                core::utils::timestampToString(start_query), core::utils::timestampToString(end_query));
                    fetched_candle_data = db_manager.queryCandles(test_instrument, test_interval, start_query, end_query); // Assign to outer variable
                    logger->info("queryCandles returned {} candles.", fetched_candle_data.size());
                } catch (const std::exception& e) {
                    logger->error("Exception during queryCandles test: {}", e.what());
                }
                logger->info("--- queryCandles Test Complete ---");


                // --- Test SmaIndicator ---
                logger->info("--- Testing SmaIndicator ---");
                try {
                    std::string test_instrument = "NSE_EQ|INE002A01018"; // Use known good params
                    std::string test_interval = "day";

                    // --- ADJUST DATE RANGE FOR MORE DATA ---
                    // Fetch ~3 months of data starting near the beginning of known data
                    core::Timestamp start_sma_test = core::utils::stringToTimestamp("2020-04-20T00:00:00+05:30"); // Known start
                    core::Timestamp end_sma_test   = core::utils::stringToTimestamp("2020-07-31T23:59:59+05:30"); // ~3 months later
                    // --- END DATE ADJUSTMENT ---

                    logger->info("Fetching data for SMA test: {} ({}) from {} to {}",
                                    test_instrument, test_interval,
                                    core::utils::timestampToString(start_sma_test),
                                    core::utils::timestampToString(end_sma_test));

                    auto candle_data = db_manager.queryCandles(test_instrument, test_interval, start_sma_test, end_sma_test); // Fetch data
                    logger->info("Got {} candles for SMA test.", candle_data.size());

                    // Check if we have enough data (e.g., > period)
                    int sma_period = 10; // Example: Calculate SMA(10)
                    // Make check slightly more robust: need at least 'period' candles for 1 result
                    if (candle_data.size() >= static_cast<size_t>(sma_period)) {
                        indicators::SmaIndicator sma10(sma_period);

                        logger->info("Calculating {}...", sma10.getName());
                        sma10.calculate(candle_data); // Calculate SMA

                        const auto& sma_results = sma10.getResult();
                        int expected_results = static_cast<int>(candle_data.size()) - sma10.getLookback();
                        if (expected_results < 0) expected_results = 0;

                        logger->info("{} calculation complete. Expected results: {}, Got results: {}",
                                        sma10.getName(), expected_results, sma_results.size());

                        // ... (rest of logging results remains the same) ...
                            if (sma_results.size() != static_cast<size_t>(expected_results)) {
                                logger->warn("SMA result size mismatch! Lookback: {}", sma10.getLookback());
                            }
                            size_t num_results_to_log = std::min<size_t>(5, sma_results.size());
                            // ... (logging loop) ...

                    } else {
                        logger->warn("Not enough candle data fetched ({}) to perform SMA({}) test.", candle_data.size(), sma_period);
                    }
                } catch (const std::exception& e) {
                    logger->error("Exception during SmaIndicator test: {}", e.what());
                }
                logger->info("--- SmaIndicator Test Complete ---");


                // --- Test UpstoxApiClient (API Fetch & DB Write) ---
                // (Keeping this test block as well)
                logger->info("--- Testing UpstoxApiClient ---");
                if (has_api_credentials) {
                    data::UpstoxApiClient upstox_client(api_key, api_secret, redirect_uri, access_token);
                    try {
                        logger->info("--- Testing getHistoricalCandleData ---");
                        std::string api_instrument = "NSE_EQ|INE002A01018";
                        std::string api_interval = "day";
                        auto t_now = std::time(nullptr);
                        auto t_5days_ago = t_now - 5 * 24 * 60 * 60;
                        std::tm now_tm, past_tm;
                        #ifdef _WIN32
                            gmtime_s(&now_tm, &t_now); gmtime_s(&past_tm, &t_5days_ago);
                        #else
                            gmtime_r(&t_now, &now_tm); gmtime_r(&t_5days_ago, &past_tm);
                        #endif
                        std::string to_date_str(11, '\0');
                        std::strftime(&to_date_str[0], to_date_str.size(), "%Y-%m-%d", &now_tm);
                        to_date_str.pop_back();
                        std::string from_date_str(11, '\0');
                        std::strftime(&from_date_str[0], from_date_str.size(), "%Y-%m-%d", &past_tm);
                        from_date_str.pop_back();

                        logger->info("Requesting Upstox data: {} / {} / {} -> {}",
                                      api_instrument, api_interval, from_date_str, to_date_str);

                        core::TimeSeries<core::Candle> api_candles = upstox_client.getHistoricalCandleData(
                            api_instrument, api_interval, from_date_str, to_date_str);

                        logger->info("Upstox API returned {} candles.", api_candles.size());

                        if (!api_candles.empty()) {
                            logger->info("Attempting to save {} fetched candles to DB...", api_candles.size());
                            if (db_manager.saveCandles(api_candles, api_instrument, api_interval)) {
                                logger->info("Successfully saved/ignored fetched candles.");
                            } else {
                                logger->error("Failed to save fetched candles.");
                            }
                        }
                        logger->info("--- getHistoricalCandleData Test Complete ---");
                    } catch (const std::exception& e) {
                        logger->error("Exception during UpstoxApiClient test: {}", e.what());
                    }
                } else {
                    logger->warn("Skipping UpstoxApiClient test due to missing credentials.");
                }
                logger->info("--- UpstoxApiClient Test Complete ---");

            } else { // This else belongs to initializeSchema check
                logger->error("DB schema initialization check failed.");
            }

            // Disconnect happens after all tests within the connection block
            db_manager.disconnect();
            logger->info("DB disconnected.");

        } else { // This else belongs to connect check
            logger->critical("DB connection failed. Cannot proceed with tests.");
        }
        logger->info("--- Tests Complete ---");

        // ======================================================
        // --- Run Backtest ---
        // ======================================================
        logger->info("---=== Starting Backtest Run ===---");

        // 1. Load Strategy Config from JSON
        std::string strategy_file_path = "strategies/simple_sma_trend.json";
        using json = nlohmann::json; // Alias for convenience

        json strategy_config; // Use using alias from factory header? No, define here.

        logger->info("Loading strategy config from: {}", strategy_file_path);
        try {
            std::ifstream ifs(strategy_file_path);
            if (!ifs.is_open()) {
                throw std::runtime_error(fmt::format("Failed to open strategy file: {}", strategy_file_path));
            }
            strategy_config = json::parse(ifs);
            ifs.close();
            logger->info("Strategy config loaded successfully.");
        } catch (const std::exception& e) {
             logger->critical("Failed to load or parse strategy config file '{}': {}", strategy_file_path, e.what());
             return 1; // Exit if config cannot be loaded
        }

        // 2. Define Backtest Parameters
        // TODO: Get these from command line args later
        double initial_capital = 100000.0; // Example: 1 Lakh
        std::string start_date = "2016-01-01"; // Example Start Date (adjust to available data)
        std::string end_date = "2016-12-31";   // Example End Date (adjust to available data)
        logger->info("Backtest Parameters: Capital={:.2f}, Start={}, End={}", initial_capital, start_date, end_date);

        // 3. Create and Run Backtester
        // Note: db_manager connection is handled inside backtester.run now
        backtester::Backtester the_backtester(db_manager, initial_capital);
        bool success = the_backtester.run(strategy_config, start_date, end_date);

        if (success) {
             logger->info("---=== Backtest Run Finished Successfully ===---");
             // TODO: Print metrics from the_backtester.getMetrics() later
        } else {
              logger->error("---=== Backtest Run Failed ===---");
        }
        // ======================================================
        // --- End Backtest Run ---
        // ======================================================

        logger->info("Trading Platform CLI finished.");

    // --- Exception Handling ---
    } catch (const core::TradingPlatformException& ex) {
        std::cerr << "Platform Error: " << ex.what() << std::endl;
         if(logger) logger->critical("Platform Error: {}", ex.what());
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Standard Error: " << ex.what() << std::endl;
         if(logger) logger->critical("Standard Error: {}", ex.what());
        return 1;
    } catch (...) {
        std::cerr << "Unknown Error occurred." << std::endl;
        if(logger) logger->critical("Unknown Error occurred.");
        return 1;
    }

    // Success
    return 0;
}
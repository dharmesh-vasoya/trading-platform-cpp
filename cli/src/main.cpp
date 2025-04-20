// cli/src/main.cpp

// Standard includes
#include <iostream>
#include <string>      // Needed for std::string path
#include <vector>      // Needed for TimeSeries/vector
#include <exception>   // Needed for std::exception
#include <chrono>      // Needed for creating test timestamps
#include <cstdlib>     // Needed for std::getenv

// Project includes
#include "logging.hpp"        // For logging functionality
#include "exceptions.hpp"     // For custom exception types
#include "datatypes.hpp"      // For core::Candle, core::Timestamp, etc.
#include "utils.hpp"          // For timestampToString/stringToTimestamp helpers
#include "database_manager.hpp" // Include the DB manager
#include "upstox_api_client.hpp"// <<<--- ADDED THIS INCLUDE

int main(int argc, char* argv[]) 
{
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
            // Keep has_api_credentials true if key/secret exist, as token might be generated later
            // Set to false for now if token is mandatory for testing
            has_api_credentials = false;
        }
         if (redirect_uri.empty() || redirect_uri == "YOUR_CONFIGURED_REDIRECT_URI") {
            logger->warn("Redirect URI is not set or using placeholder.");
            // Treat as missing credentials if needed for auth flow later
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

                // --- Test queryCandles (DB Read) ---
                logger->info("--- Testing queryCandles ---");
                try {
                    std::string test_instrument = "NSE_EQ|INE002A01018"; // Use known good params
                    std::string test_interval = "day";
                    core::Timestamp start_query = core::utils::stringToTimestamp("2015-04-20T00:00:00+05:30"); // Use known good params
                    core::Timestamp end_query   = core::utils::stringToTimestamp("2015-04-24T23:59:59+05:30");

                    logger->info("Querying DB candles for {} ({}) from {} to {}",
                                test_instrument, test_interval,
                                core::utils::timestampToString(start_query), core::utils::timestampToString(end_query));
                    auto db_candles = db_manager.queryCandles(test_instrument, test_interval, start_query, end_query);
                    logger->info("queryCandles returned {} candles.", db_candles.size());
                    // Maybe add back first/last logging if needed
                } catch (const std::exception& e) {
                    logger->error("Exception during queryCandles test: {}", e.what());
                }
                logger->info("--- queryCandles Test Complete ---");


                // --- Test UpstoxApiClient (API Fetch & DB Write) ---
                logger->info("--- Testing UpstoxApiClient ---");
                if (has_api_credentials) {
                    data::UpstoxApiClient upstox_client(api_key, api_secret, redirect_uri, access_token); // Define client here
                    try {
                        logger->info("--- Testing getHistoricalCandleData ---");
                        std::string api_instrument = "NSE_EQ|INE002A01018"; // Use valid key
                        std::string api_interval = "day"; // Use API supported interval
                        // Use recent dates
                        auto t_now = std::time(nullptr);
                        auto t_5days_ago = t_now - 5 * 24 * 60 * 60;
                        std::tm now_tm, past_tm;
                        #ifdef _WIN32 // Use gmtime_s on Windows
                            gmtime_s(&now_tm, &t_now);
                            gmtime_s(&past_tm, &t_5days_ago);
                        #else // Use gmtime_r on POSIX
                            gmtime_r(&t_now, &now_tm);
                            gmtime_r(&t_5days_ago, &past_tm);
                        #endif

                        std::string to_date_str(11, '\0');
                        std::strftime(&to_date_str[0], to_date_str.size(), "%Y-%m-%d", &now_tm);
                        to_date_str.pop_back();

                        std::string from_date_str(11, '\0');
                        std::strftime(&from_date_str[0], from_date_str.size(), "%Y-%m-%d", &past_tm);
                        from_date_str.pop_back();

                        logger->info("Requesting Upstox data: {} / {} / {} -> {}",
                                    api_instrument, api_interval, from_date_str, to_date_str);

                        // *** VERIFY API Endpoint and JSON structure in upstox_api_client.cpp against V2 Docs! ***
                        core::TimeSeries<core::Candle> api_candles = upstox_client.getHistoricalCandleData(
                            api_instrument, api_interval, from_date_str, to_date_str);

                        logger->info("Upstox API returned {} candles.", api_candles.size());

                        // Optional: Try saving the fetched candles to the DB
                        if (!api_candles.empty()) {
                            logger->info("Attempting to save {} fetched candles to DB...", api_candles.size());
                            // db_manager is still in scope here
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

        // --- Core Logic (Placeholder) ---
        logger->warn("No backtesting logic implemented yet.");
        logger->info("Trading Platform CLI finished.");

    // --- Exception Handling ---
    // Keep existing catch blocks here...
} catch (const core::TradingPlatformException& ex) {
    std::cerr << "Platform Error: " << ex.what() << std::endl;
     if(logger) logger->critical("Platform Error: {}", ex.what()); // Check if logger is valid
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
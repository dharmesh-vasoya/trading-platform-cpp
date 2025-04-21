#include "backtester.hpp"
#include "strategy_factory.hpp" // Need factory to create strategy
#include "sma_indicator.hpp"    // Need concrete indicators for createIndicator factory <<< USE SHORT PATH
// Add other indicator headers later (rsi_indicator.hpp etc.)
#include "logging.hpp"          // <<< USE SHORT PATH
#include "utils.hpp"            // <<< USE SHORT PATH
#include "exceptions.hpp"       // <<< USE SHORT PATH

#include <stdexcept>
#include <iostream>
#include <regex>       // For parsing indicator names
#include <string>      // For std::stoi
#include <utility>     // For std::pair

namespace backtester {

    Backtester::Backtester(data::DatabaseManager& db_manager, double initial_capital)
        : db_manager_(db_manager), initial_capital_(initial_capital)
    {
        portfolio_ = std::make_unique<Portfolio>(initial_capital_);
        core::logging::getLogger()->debug("Backtester initialized with capital: {}", initial_capital_);
    }

    bool Backtester::run(const json& strategy_config,
                         const std::string& start_date,
                         const std::string& end_date)
    {
        auto logger = core::logging::getLogger();
        logger->info("Starting backtest run...");
        // ... (Basic steps - load strategy, load data, create indicators, run loop, calc metrics) ...
        // ... (Return true/false based on success) ...
         logger->warn("Backtester::run - NOT FULLY IMPLEMENTED");
         return false; // Placeholder
    }

    bool Backtester::loadData(const std::string& start_date, const std::string& end_date) {
         core::logging::getLogger()->warn("Backtester::loadData - NOT IMPLEMENTED");
         return false; // Placeholder
    }

    // Helper to parse indicator name and period (e.g., "SMA(10)")
    // Returns {name, period} or {name, -1} if no period found
    // TODO: Move this to a common indicator utility?
    inline std::pair<std::string, int> parseIndicatorString(const std::string& indicator_str) {
         static const std::regex indicator_regex(R"((.+)\((\d+)\))"); // e.g., SMA(10)
         std::smatch match;
         if (std::regex_match(indicator_str, match, indicator_regex) && match.size() == 3) {
             try {
                 // Convert base name to uppercase for consistent matching? Optional.
                 // std::string base_name = match[1].str();
                 // std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);
                 // return {base_name, std::stoi(match[2].str())};
                  return {match[1].str(), std::stoi(match[2].str())};
             } catch (...) { /* ignore stoi error */ }
         }
         // Handle names without parentheses (e.g., "ClosePrice") if needed
         return {indicator_str, -1}; // Return full string and -1 if no period found/parsed
     }

    std::unique_ptr<indicators::IIndicator> Backtester::createIndicator(const std::string& name) {
         auto logger = core::logging::getLogger();
         logger->debug("Attempting to create indicator instance for: {}", name);

         auto parsed = parseIndicatorString(name);
         std::string base_name = parsed.first;
         int period = parsed.second;

         // Simple factory logic (expand later with more indicators)
         if (base_name == "SMA") {
             if (period <= 0) {
                 logger->error("Invalid period {} for SMA indicator '{}'", period, name);
                 return nullptr;
             }
             logger->debug("Creating SMAIndicator({})", period);
             return std::make_unique<indicators::SmaIndicator>(period);
         }
         // --- Add cases for other indicators ---
         // else if (base_name == "RSI") {
         //    if (period <= 0) { /* error */ return nullptr; }
         //    logger->debug("Creating RsiIndicator({})", period);
         //    return std::make_unique<indicators::RsiIndicator>(period); // Requires RsiIndicator implementation
         // }
         // else if (base_name == "MACD") { ... need multiple periods ... }

         logger->error("Unknown indicator name requested by strategy factory: {}", name);
         return nullptr; // Failed to create
    }

    bool Backtester::createAndCalculateIndicators() {
         core::logging::getLogger()->warn("Backtester::createAndCalculateIndicators - NOT IMPLEMENTED");
          return false; // Placeholder
    }

    void Backtester::runEventLoop() {
         core::logging::getLogger()->warn("Backtester::runEventLoop - NOT IMPLEMENTED");
    }

    void Backtester::executeSignal(core::Timestamp timestamp, const core::Candle& current_candle, core::SignalAction signal) {
          core::logging::getLogger()->warn("Backtester::executeSignal - NOT IMPLEMENTED");
    }

    void Backtester::calculateMetrics() {
         core::logging::getLogger()->warn("Backtester::calculateMetrics - NOT IMPLEMENTED");
    }

    // Getter added for completeness, might need adjustment
    const Portfolio& Backtester::getPortfolio() const {
         if (!portfolio_) {
              throw std::runtime_error("Portfolio accessed before initialization in backtester.");
         }
         return *portfolio_;
    }


} // namespace backtester
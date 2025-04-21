#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp> // For strategy config

// Required project headers (use short paths)
#include "datatypes.hpp"
#include "database_manager.hpp" // Needs DatabaseManager definition
#include "interfaces.hpp"       // Strategy engine interfaces
#include "indicators.hpp"       // Indicator interface
#include "portfolio.hpp"        // Portfolio class

// Forward declare specific indicator classes needed for creation
// Alternatively, include them all or use a factory later
namespace indicators { class SmaIndicator; } // Example

namespace backtester {

    using json = nlohmann::json;

    class Backtester {
    public:
        // Constructor requires a database connection reference
        explicit Backtester(data::DatabaseManager& db_manager, double initial_capital = 100000.0);

        // Main execution function
        bool run(const json& strategy_config,
                 const std::string& start_date, // Format: YYYY-MM-DD
                 const std::string& end_date);   // Format: YYYY-MM-DD

        // --- TODO: Add methods to get results ---
        const Portfolio& getPortfolio() const; // Return portfolio details
        // BacktestMetrics getMetrics() const; // Define BacktestMetrics struct

    private:
        data::DatabaseManager& db_manager_; // Use reference, doesn't own it
        double initial_capital_;
        std::unique_ptr<Portfolio> portfolio_;
        std::unique_ptr<strategy_engine::IStrategy> strategy_;

        // Store required indicators, mapped by name (e.g., "SMA(10)")
        std::map<std::string, std::unique_ptr<indicators::IIndicator>> indicators_;
        // Store loaded historical data (simplified: one primary instrument/timeframe)
        core::TimeSeries<core::Candle> primary_data_;
        std::string primary_instrument_key_; // Store which instrument was loaded
        // Store calculated indicator results aligned with primary_data_
         std::map<std::string, core::TimeSeries<double>> indicator_results_;


        // --- Private Helper Methods ---
        bool loadData(const std::string& start_date, const std::string& end_date);
        bool createAndCalculateIndicators();
        void runEventLoop();
        void executeSignal(core::Timestamp timestamp, const core::Candle& current_candle, core::SignalAction signal);
        void calculateMetrics();
        // Helper to create indicator instances (simple version)
        std::unique_ptr<indicators::IIndicator> createIndicator(const std::string& name);
    };

} // namespace backtester
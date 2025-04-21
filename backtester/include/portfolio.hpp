#pragma once

#include <string>
#include <vector>
#include <map>
#include "datatypes.hpp" // Using short path for core::Timestamp

namespace backtester {

    // Basic portfolio state tracking at a point in time
    struct PortfolioState {
        core::Timestamp timestamp;
        double cash = 0.0;
        double positions_value = 0.0; // Market value of all holdings
        double total_equity = 0.0;   // cash + positions_value
    };

    class Portfolio {
    public:
        explicit Portfolio(double initial_capital);

        double getCash() const;
        long long getPositionQuantity(const std::string& instrument_key) const;
        double getCurrentEquity(const std::map<std::string, double>& current_prices) const; // Recalculate equity
        const std::vector<PortfolioState>& getEquityCurve() const;

        // TODO: Implement logic later
        void recordTrade(core::Timestamp timestamp,
                         const std::string& instrument_key,
                         core::SignalAction action,
                         long long quantity,
                         double execution_price,
                         double commission = 0.0);

        // Records equity at a specific time point based on current prices
        void recordTimestampValue(core::Timestamp timestamp, const std::map<std::string, double>& current_prices);

    private:
        double initial_capital_;
        double cash_;
        // Map: Instrument Key -> Quantity held (positive=long, negative=short)
        std::map<std::string, long long> positions_;
        std::vector<PortfolioState> equity_curve_; // Track equity over time
    };

} // namespace backtester
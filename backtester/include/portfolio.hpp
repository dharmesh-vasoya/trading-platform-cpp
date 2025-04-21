// backtester/include/portfolio.hpp
#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional> // Required for std::optional if not included via datatypes.hpp already

// Use short paths
#include "datatypes.hpp" // Provides core::Timestamp, core::SignalAction, core::Trade
#include "logging.hpp"   // Provides core::logging::getLogger needed by BacktestMetrics::logMetrics

namespace backtester {

    // --- Backtest Metrics Struct ---
    struct BacktestMetrics {
        double total_return_pct = 0.0;
        double max_drawdown_pct = 0.0;
        double total_pnl = 0.0;
        int total_executions = 0;     // Renamed from total_trades for clarity
        int round_trip_trades = 0;
        double win_rate = 0.0;        // Based on round trips
        double profit_factor = 0.0;   // Gross Profit / Gross Loss
        double avg_win_pnl = 0.0;
        double avg_loss_pnl = 0.0;
        // Add more later...

        // Helper method to log calculated metrics
        void logMetrics() const {
            // Ensure logger is available. Consider passing logger or using static access if guaranteed initialized.
            auto logger = core::logging::getLogger();
            logger->info("--- Backtest Metrics ---");
            logger->info("Total Return: {:.2f}%", total_return_pct * 100.0);
            logger->info("Total PnL: {:.2f}", total_pnl);
            logger->info("Max Drawdown: {:.2f}%", max_drawdown_pct * 100.0);
            logger->info("Total Executions: {}", total_executions);
            logger->info("Round-Trip Trades: {}", round_trip_trades);
            logger->info("Win Rate: {:.2f}%", win_rate * 100.0);
            logger->info("Profit Factor: {:.2f}", profit_factor);
            logger->info("Avg Win PnL: {:.2f}", avg_win_pnl);
            logger->info("Avg Loss PnL: {:.2f}", avg_loss_pnl);
            logger->info("------------------------");
        }
    };

    // --- Portfolio State Struct (for equity curve) ---
    struct PortfolioState {
        core::Timestamp timestamp;
        double cash = 0.0;
        double positions_value = 0.0; // Market value of all holdings
        double total_equity = 0.0;   // cash + positions_value
    };

    // --- Open Position Info Struct --- (Defined before Portfolio class)
    // Stores details needed to calculate PnL when a position is closed
    struct OpenPositionInfo {
        core::Timestamp entry_time;
        double entry_price = 0.0;     // Avg entry price for this open position
        long long entry_quantity = 0; // Signed quantity (+long, -short) held
        double entry_commission = 0.0;// Commission paid on entry
    };

    // --- Portfolio Class Definition ---
    class Portfolio {
    public:
        explicit Portfolio(double initial_capital);

        // --- Getters ---
        double getCash() const;
        long long getPositionQuantity(const std::string& instrument_key) const;
        // Calculates current equity based on cash and market value of open positions
        double getCurrentEquity(const std::map<std::string, double>& current_prices) const;
        const std::vector<PortfolioState>& getEquityCurve() const;
        int getTotalExecutions() const { return execution_count_; } // Use updated member name
        const std::vector<core::Trade>& getTradeLog() const;      // Getter for completed trades

        // --- Modifiers ---
        // Records an execution, updates cash/positions, logs completed trades
        void recordTrade(core::Timestamp timestamp,
                         const std::string& instrument_key,
                         core::SignalAction action, // Signal that caused trade
                         long long quantity,        // Always positive quantity executed
                         double execution_price,
                         double commission = 0.0); // Commission for THIS execution leg

        // Records portfolio equity state at a specific timestamp
        void recordTimestampValue(core::Timestamp timestamp, const std::map<std::string, double>& current_prices);

    private:
        double initial_capital_;
        double cash_;
        // Map: Instrument Key -> Current Quantity held (+long / -short)
        std::map<std::string, long long> positions_;
        // Map: Instrument Key -> Info about the currently open position
        std::map<std::string, OpenPositionInfo> open_positions_info_;
        // Vector storing historical portfolio state (for equity curve / drawdown)
        std::vector<PortfolioState> equity_curve_;
        // Counter for total buy/sell executions
        int execution_count_ = 0;
        // Vector storing details of completed round-trip trades
        std::vector<core::Trade> trade_log_;
    };

} // namespace backtester
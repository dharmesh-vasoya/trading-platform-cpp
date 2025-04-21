#include "portfolio.hpp"
#include "logging.hpp" // <<<--- ADD THIS
#include "utils.hpp"   // <<<--- ADD THIS
#include <stdexcept> // For invalid_argument
#include <numeric>   // For std::accumulate (used in getCurrentEquity)

namespace backtester {


    Portfolio::Portfolio(double initial_capital)
        : initial_capital_(initial_capital), cash_(initial_capital) {
        if (initial_capital <= 0) {
            throw std::invalid_argument("Initial capital must be positive.");
        }
        // Optional: Record initial state at time zero?
    }

    double Portfolio::getCash() const {
        return cash_;
    }

    long long Portfolio::getPositionQuantity(const std::string& instrument_key) const {
         auto it = positions_.find(instrument_key);
         return (it != positions_.end()) ? it->second : 0;
     }

    // Calculates total equity based on current cash and market value of positions
    double Portfolio::getCurrentEquity(const std::map<std::string, double>& current_prices) const {
        double total_position_value = 0.0;
        for(const auto& pair : positions_) {
             const std::string& key = pair.first;
             long long quantity = pair.second;
             auto price_it = current_prices.find(key);
             if (price_it != current_prices.end()) {
                 total_position_value += quantity * price_it->second;
             } else {
                 // Price missing for a held position - this is problematic!
                 // For now, we might have to assume it contributes zero or throw error.
                 // Logging handled elsewhere perhaps.
             }
        }
        return cash_ + total_position_value;
    }

    const std::vector<PortfolioState>& Portfolio::getEquityCurve() const {
        return equity_curve_;
    }

    // Records the portfolio state at a specific timestamp
    void Portfolio::recordTimestampValue(core::Timestamp timestamp, const std::map<std::string, double>& current_prices) {
        // Avoid duplicate entries for the same timestamp
        if (equity_curve_.empty() || equity_curve_.back().timestamp != timestamp) {
            PortfolioState current_state;
            current_state.timestamp = timestamp;
            current_state.cash = cash_;

            double pos_value = 0.0;
            for(const auto& pair : positions_) {
                auto price_it = current_prices.find(pair.first);
                if (price_it != current_prices.end()) {
                    pos_value += pair.second * price_it->second;
                }
            }
            current_state.positions_value = pos_value;
            current_state.total_equity = current_state.cash + current_state.positions_value;
            equity_curve_.push_back(current_state);
        }
    }

    void Portfolio::recordTrade(core::Timestamp timestamp,
        const std::string& instrument_key,
        core::SignalAction action,
        long long quantity, // Assumed positive from executeSignal
        double execution_price,
        double commission) // Commission for THIS execution leg
    {
        auto logger = core::logging::getLogger();
        if (quantity <= 0) {
        logger->warn("Attempted to record trade with zero or negative quantity: {}", quantity);
        return;
        }

        long long current_qty = getPositionQuantity(instrument_key);
        long long position_change = 0;
        double cost = 0.0; // Net change in cash
        bool is_entry = false;
        bool is_exit = false;

        switch(action) {
            case core::SignalAction::EnterLong: { // <-- Add brace
                if (current_qty < 0) { logger->warn("Cannot EnterLong while Short in {}. Ignoring.", instrument_key); return; }
                if (current_qty == 0) is_entry = true;
                position_change = quantity;
                cost = -(quantity * execution_price) - commission;
                break;
            } // <-- Add brace
    
            case core::SignalAction::ExitLong: { // <-- Add brace
                if (current_qty <= 0) { logger->warn("Cannot ExitLong if not Long in {}. Ignoring.", instrument_key); return; }
                if (quantity > current_qty) { quantity = current_qty; }
                position_change = -quantity;
                cost = (quantity * execution_price) - commission;
                if (current_qty + position_change == 0) is_exit = true; // Correct check if flat
                break;
            } // <-- Add brace
    
            case core::SignalAction::EnterShort: { // <-- Add brace
                 if (current_qty > 0) { logger->warn("Cannot EnterShort while Long in {}. Ignoring.", instrument_key); return; }
                 if (current_qty == 0) is_entry = true;
                 position_change = -quantity;
                 cost = (quantity * execution_price) - commission;
                break;
            } // <-- Add brace
    
            case core::SignalAction::ExitShort: { // <<<--- ADD BRACE HERE ---
                 if (current_qty >= 0) { logger->warn("Cannot ExitShort if not Short in {}. Ignoring.", instrument_key); return; }
                 long long abs_current_qty = -current_qty; // Initialization is now safe inside scope
                 if (quantity > abs_current_qty) { quantity = abs_current_qty;}
                 position_change = quantity;
                 cost = -(quantity * execution_price) - commission;
                 if (current_qty + quantity == 0) is_exit = true; // Correct check if flat
                break;
             } // <<<--- ADD BRACE HERE ---
    
            case core::SignalAction::None: // Fall through to default is fine if no action needed
            default: { // <-- Add brace
                logger->warn("recordTrade called with invalid or None action: {}", static_cast<int>(action));
                return; // Return here as no valid trade action
            } // <-- Add brace
        } // End switch

        // Check sufficient cash for buys/covers
        if (cost < 0 && cash_ + cost < 0) {
        logger->error("Insufficient cash for trade! Have: {:.2f}, Need: {:.2f}. Trade ignored.", cash_, -cost);
        return;
        }

        // --- Update Portfolio State ---
        cash_ += cost;
        positions_[instrument_key] += position_change;
        execution_count_++; // Count every execution

        logger->info("Trade Executed: Time={}, Inst={}, Action={}, Qty={}, Price={:.2f}, Comm={:.2f}, Cost={:.2f}, NewCash={:.2f}, NewPosQty={}",
        core::utils::timestampToString(timestamp), // Requires utils.hpp include
        instrument_key,
        static_cast<int>(action),
        position_change, // Log the actual change +/- quantity
        execution_price,
        commission,
        cost,
        cash_,
        getPositionQuantity(instrument_key)
        );

        // --- Log Completed Trade ---
        if (is_exit) {
        auto it_info = open_positions_info_.find(instrument_key);
        if (it_info != open_positions_info_.end()) {
        // Calculate PnL for this round trip
        const auto& entry_info = it_info->second;
        double entry_value = entry_info.entry_quantity * entry_info.entry_price;
        // Note: position_change here is negative for long exit, positive for short exit
        double exit_value = (-position_change) * execution_price; // Value of shares sold/bought back
        double total_commission = entry_info.entry_commission + commission; // Sum commissions

        core::Trade trade;
        trade.instrument_key = instrument_key;
        trade.entry_action = (entry_info.entry_quantity > 0) ? core::SignalAction::EnterLong : core::SignalAction::EnterShort;
        trade.entry_time = entry_info.entry_time;
        trade.exit_time = timestamp;
        trade.quantity = entry_info.entry_quantity; // Store absolute entry quantity
        trade.entry_price = entry_info.entry_price;
        trade.exit_price = execution_price;
        trade.commission = total_commission;

        if (trade.entry_action == core::SignalAction::EnterLong) {
        trade.pnl = exit_value - entry_value - total_commission;
        } else { // Short sale
        trade.pnl = entry_value - exit_value - total_commission; // (Sell high - Buy low) - commission
        }
        trade.return_pct = (entry_value != 0) ? trade.pnl / std::abs(entry_value) : 0.0;

        trade_log_.push_back(trade);
        logger->debug("Round Trip Trade Logged: PnL = {:.2f}", trade.pnl);

        // Clean up open position info
        open_positions_info_.erase(it_info);
        } else {
        logger->warn("Exited position for {} but no entry info found.", instrument_key);
        }
        } else if (is_entry) {
        // Store entry info for later PnL calculation on exit
        OpenPositionInfo entry_info;
        entry_info.entry_time = timestamp;
        entry_info.entry_price = execution_price;
        entry_info.entry_quantity = position_change; // Store signed quantity (+ for long, - for short)
        entry_info.entry_commission = commission;
        open_positions_info_[instrument_key] = entry_info;
        logger->debug("Entry info recorded for {}. Qty: {}, Price: {:.2f}", instrument_key, entry_info.entry_quantity, entry_info.entry_price);
        }
        // Handle partial exits / pyramiding later if needed by adjusting OpenPositionInfo

        // Remove instrument from main positions_ map if flat
        if (positions_[instrument_key] == 0) {
        positions_.erase(instrument_key);
        // Ensure open info is also removed if somehow missed by is_exit logic
        open_positions_info_.erase(instrument_key);
        }
    }

    const std::vector<core::Trade>& Portfolio::getTradeLog() const {
        // Simply return the member variable
        return trade_log_;
    }

} // namespace backtester

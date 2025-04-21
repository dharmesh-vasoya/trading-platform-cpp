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
        core::SignalAction action, // Use this to determine buy/sell direction if quantity isn't signed
        long long quantity, // Let's assume positive quantity, action determines direction
        double execution_price,
        double commission)
    {
    if (quantity <= 0) {
    core::logging::getLogger()->warn("Attempted to record trade with zero or negative quantity: {}", quantity);
    return;
    }

    double trade_value = quantity * execution_price;
    double cost = 0.0;
    long long position_change = 0;

    switch(action) {
    case core::SignalAction::EnterLong:
    case core::SignalAction::ExitShort: // Covering short is effectively a buy
    cost = -trade_value - commission; // Money out
    position_change = quantity;       // Position increases
    break;
    case core::SignalAction::EnterShort:
    case core::SignalAction::ExitLong: // Selling long is effectively a sell
    cost = trade_value - commission; // Money in
    position_change = -quantity;     // Position decreases
    break;
    case core::SignalAction::None:
    default:
    core::logging::getLogger()->warn("recordTrade called with invalid action: {}", static_cast<int>(action));
    return;
    }

    // Basic checks (can enhance later)
    if (cash_ + cost < 0) {
        core::logging::getLogger()->error("Insufficient cash for trade! Cash: {:.2f}, Cost: {:.2f}. Trade ignored.", cash_, cost);
        // In a real system, handle margin, order rejection etc.
        return;
    }

    // Update portfolio
    cash_ += cost;
    positions_[instrument_key] += position_change;
    trade_count_++;

    // Remove instrument from map if position is flat
    if (positions_[instrument_key] == 0) {
    positions_.erase(instrument_key);
    }

    core::logging::getLogger()->info("Trade Recorded: Time={}, Inst={}, Action={}, Qty={}, Price={:.2f}, Comm={:.2f}, Cost={:.2f}, NewCash={:.2f}, NewPosQty={}",
    core::utils::timestampToString(timestamp), // Need utils include in portfolio.cpp? Or pass string
    instrument_key,
    static_cast<int>(action), // Log enum value for now
    position_change, // Log the change +/- quantity
    execution_price,
    commission,
    cost,
    cash_,
    getPositionQuantity(instrument_key) // Log final position
    );

    // Optional: Record equity again immediately after trade?
    // std::map<std::string, double> prices = {{instrument_key, execution_price}};
    // recordTimestampValue(timestamp, prices);
    }


} // namespace backtester
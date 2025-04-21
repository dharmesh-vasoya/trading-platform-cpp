#include "portfolio.hpp"
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

    // --- TODO: Implement recordTrade ---
    void Portfolio::recordTrade(core::Timestamp timestamp,
                               const std::string& instrument_key,
                               core::SignalAction action, // Not directly used yet, logic based on quantity sign
                               long long quantity, // Positive for buy, Negative for sell
                               double execution_price,
                               double commission)
    {
         // Needs careful implementation:
         // 1. Check if quantity is non-zero
         // 2. Calculate trade value = quantity * execution_price
         // 3. Calculate cost = trade value (negative for buy, positive for sell) - commission
         // 4. Check if enough cash for buys (cash_ + cost >= 0 ?) -> Handle margin later
         // 5. Update cash_: cash_ += cost
         // 6. Update positions_: positions_[instrument_key] += quantity
         // 7. If positions_[instrument_key] becomes 0, maybe remove key from map?
         // 8. Record the trade details somewhere (e.g., a trade log vector)
    }


} // namespace backtester
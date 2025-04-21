#pragma once // Use #pragma once for include guards (common practice)

#include <string>
#include <vector>
#include <chrono> // For timestamps
#include <map>    // For positions
#include <optional> // For nullable fields like open_interest

namespace core {

    // Using system_clock for time points, can be adjusted if needed
    using Timestamp = std::chrono::system_clock::time_point;


    struct Candle {
        Timestamp timestamp;
        double open = 0.0;
        double high = 0.0;
        double low = 0.0;
        double close = 0.0;
        long long volume = 0; // Use long long for potentially large volumes
        std::optional<long long> open_interest; // Optional for non-futures/options

        // Add comparison operators if needed for sorting/lookup
        bool operator<(const Candle& other) const {
            return timestamp < other.timestamp;
        }
    };

    enum class SignalAction {
        None,
        EnterLong,
        ExitLong,
        EnterShort,
        ExitShort
    };

    // Represents the current state of a strategy's position
    enum class PositionState {
        None,  // Flat, no position
        Long,  // Currently holding a long position
        Short  // Currently holding a short position
    };

    struct Signal {
        Timestamp timestamp;
        std::string instrument_key;
        SignalAction action = SignalAction::None;
        double suggested_price = 0.0; // Price at the time signal triggered
        std::string strategy_id; // Identify which strategy generated it
    };

    struct Order {
        Timestamp timestamp;
        std::string instrument_key;
        SignalAction action; // Corresponds to the signal that generated it
        long long quantity = 0;
        double fill_price = 0.0; // Actual execution price
        double commission = 0.0;
        std::string order_id; // Unique ID for the order
    };

    struct Position {
        std::string instrument_key;
        long long quantity = 0; // Positive for long, negative for short
        double average_entry_price = 0.0;
        Timestamp last_update_time;
        // Potentially add realized/unrealized PnL here
    };

    
    struct Trade {
        std::string instrument_key;
        core::SignalAction entry_action; // EnterLong or EnterShort
        core::Timestamp entry_time;
        core::Timestamp exit_time;
        long long quantity = 0;         // Absolute quantity traded
        double entry_price = 0.0;
        double exit_price = 0.0;
        double commission = 0.0;      // Total commission (entry + exit)
        double pnl = 0.0;             // Profit or Loss for this trade
        double return_pct = 0.0;      // PnL / Entry Value
    };

    // Basic TimeSeries concept (can be refined later)
    template<typename T>
    using TimeSeries = std::vector<T>; // Simple alias for now

} // namespace core
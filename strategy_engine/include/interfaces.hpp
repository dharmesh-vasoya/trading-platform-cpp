#pragma once

#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include <map>    // For potential indicator results map

// Forward declarations or include necessary core types
#include "datatypes.hpp" // Provides Candle, SignalAction, TimeSeries etc.

namespace strategy_engine {

    // Structure to hold market data for a single point in time for evaluation
    // Can be expanded later (e.g., include multiple timeframes, order book)
    struct MarketDataSnapshot {
        core::Timestamp current_time;
        const core::Candle* current_candle = nullptr; // Pointer to current primary candle
        // Map to hold indicator results (e.g., "SMA(10)" -> 123.45)
        // Using double directly for simplicity now, could use TimeSeries<double>::value_type
        std::map<std::string, double> indicator_values;
        // TODO: Add access to previous candles/indicators if needed by conditions
    };

    // --- Condition Interface ---
    // Represents a single logical condition (e.g., price > SMA, RSI < 30)
    class ICondition {
    public:
        virtual ~ICondition() = default;
        // Evaluates the condition based on the current market snapshot
        virtual bool evaluate(const MarketDataSnapshot& snapshot) const = 0;
        // Optional: Get a description of the condition
        virtual std::string describe() const = 0;
    };

    // --- Rule Interface ---
    // Represents an entry or exit rule, typically composed of one or more conditions
    class IRule {
        public:
            virtual ~IRule() = default;
            // Evaluates the rule, returning the action if triggered, None otherwise
            virtual core::SignalAction evaluate(const MarketDataSnapshot& snapshot) const = 0;
            // Optional: Get a description of the rule
            virtual std::string describe() const = 0;
            // --- ADD THIS LINE ---
            // Get the name of the rule (implemented by derived classes like Rule)
            virtual std::string getName() const = 0;
            // --- END ADDITION ---
    };

    // --- Strategy Interface ---
    // Represents a complete trading strategy
    class IStrategy {
    public:
        virtual ~IStrategy() = default;

        // Get the unique name/ID of the strategy
        virtual std::string getName() const = 0;

        // Get list of instrument keys required by this strategy
        virtual const std::vector<std::string>& getRequiredInstruments() const = 0;

        // Get list of timeframes required (e.g., "day", "5minute")
        virtual const std::vector<std::string>& getRequiredTimeframes() const = 0;

        // Get configurations for indicators needed (e.g., map<string, params>)
        // This tells the backtester which indicators to calculate.
        // We might need a dedicated IndicatorConfig struct later.
        virtual const std::vector<std::string>& getRequiredIndicatorNames() const = 0; // Simplified for now

        // Evaluate all rules based on the current snapshot
        // Returns the triggered action (EnterLong, ExitShort, etc.) or None
        virtual core::SignalAction evaluate(const MarketDataSnapshot& snapshot) = 0;
         // Non-const because strategy might maintain internal state (e.g., trailing stops)

        // Optional: Methods for position sizing, stop loss calculation etc.
        // virtual double calculatePositionSize(double portfolio_value, double price) const = 0;
        // virtual double calculateStopLoss(double entry_price) const = 0;
    };

} // namespace strategy_engine
#pragma once

#include "interfaces.hpp" // Include the base interface
#include "common_types.hpp"
#include <string>
#include <stdexcept>
#include <variant> // To hold either a double value or a second indicator name

namespace strategy_engine {

    // --- IndicatorCondition Class ---
    // Compares an indicator's value against a fixed value OR another indicator's value.
    class IndicatorCondition : public ICondition {
    public:
        // Constructor: Compare indicator to a fixed value
        // e.g., IndicatorCondition("RSI(14)", ComparisonOp::LT, 30.0) -> "RSI(14) < 30.0"
        IndicatorCondition(const std::string& indicator_name1, ComparisonOp op, double value);

        // Constructor: Compare indicator to another indicator
        // e.g., IndicatorCondition("SMA(50)", ComparisonOp::GT, "SMA(200)") -> "SMA(50) > SMA(200)"
        IndicatorCondition(const std::string& indicator_name1, ComparisonOp op, const std::string& indicator_name2);

        virtual ~IndicatorCondition() override = default;

        // ICondition interface implementation
        bool evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

    private:
        std::string indicator_name1_;
        ComparisonOp op_;
        // Use std::variant to hold either the comparison value or the second indicator name
        std::variant<double, std::string> rhs_;
        bool compare_to_value_; // Flag to know which type is in rhs_

        // Helper
        std::string op_to_string(ComparisonOp op) const; // Can share with PriceCondition later
    };

} // namespace strategy_engine
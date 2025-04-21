#pragma once

#include "interfaces.hpp"
#include "common_types.hpp" // For PriceField, ComparisonOp
#include <string>
#include <stdexcept>

namespace strategy_engine {

    // --- PriceIndicatorCondition Class ---
    // Compares a candle price field against a named indicator's value.
    class PriceIndicatorCondition : public ICondition {
    public:
        // Constructor: e.g., PriceIndicatorCondition(PriceField::Close, ComparisonOp::GT, "SMA(10)") -> "Close > SMA(10)"
        PriceIndicatorCondition(PriceField price_field, ComparisonOp op, std::string indicator_name);

        virtual ~PriceIndicatorCondition() override = default;

        // ICondition interface implementation
        bool evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

    private:
        PriceField price_field_;
        ComparisonOp op_;
        std::string indicator_name_;

        // Helpers (can be shared later)
        double get_price_value(const core::Candle& candle, PriceField field) const;
        std::string field_to_string(PriceField field) const;
        std::string op_to_string(ComparisonOp op) const;
    };

} // namespace strategy_engine
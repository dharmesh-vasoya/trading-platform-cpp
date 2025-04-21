#pragma once

#include "interfaces.hpp" // Include the base interface
#include "common_types.hpp"
#include "datatypes.hpp"  // Include core::Candle (needed indirectly)
#include <string>
#include <stdexcept>
namespace strategy_engine {

    // --- PriceCondition Class ---
    // Compares a candle price field against a fixed value OR another price field.
    // Initial version: Compare field against a fixed value.
    class PriceCondition : public ICondition {
    public:
        // Constructor: e.g., PriceCondition(PriceField::Close, ComparisonOp::GT, 100.0) -> "Close > 100.0"
        PriceCondition(PriceField field, ComparisonOp op, double value);

        // Constructor: e.g., PriceCondition(PriceField::Close, ComparisonOp::GT, PriceField::Open) -> "Close > Open"
        PriceCondition(PriceField field1, ComparisonOp op, PriceField field2);


        virtual ~PriceCondition() override = default;

        // ICondition interface implementation
        bool evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

    private:
        PriceField field1_;
        ComparisonOp op_;
        double value_;           // Value for comparison against fixed value
        PriceField field2_;      // Field for comparison against another field
        bool compare_to_value_; // Flag to indicate which comparison type

        // Helper to get price value from candle based on enum
        double get_price_value(const core::Candle& candle, PriceField field) const;
         // Helper to convert enums to strings for describe()
         std::string field_to_string(PriceField field) const;
         std::string op_to_string(ComparisonOp op) const;
    };

} // namespace strategy_engine
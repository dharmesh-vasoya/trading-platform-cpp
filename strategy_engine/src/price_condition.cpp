#include "price_condition.hpp"
#include "logging.hpp" // Use short path now
#include "spdlog/fmt/bundled/core.h" // Use full relative path to bundled fmt header <<<--- CORRECT FIX
#include <cmath>      // For std::fabs with floating point EQ comparison

namespace strategy_engine {

// Constructor for comparing against a value
PriceCondition::PriceCondition(PriceField field, ComparisonOp op, double value)
    : field1_(field), op_(op), value_(value), field2_(PriceField::Open), // field2_ unused but needs init
      compare_to_value_(true)
{}

 // Constructor for comparing against another field
 PriceCondition::PriceCondition(PriceField field1, ComparisonOp op, PriceField field2)
     : field1_(field1), op_(op), value_(0.0), // value_ unused but needs init
       field2_(field2), compare_to_value_(false)
 {}


// Helper to get price from candle
double PriceCondition::get_price_value(const core::Candle& candle, PriceField field) const {
    switch (field) {
        case PriceField::Open:  return candle.open;
        case PriceField::High:  return candle.high;
        case PriceField::Low:   return candle.low;
        case PriceField::Close: return candle.close;
        default:
            // Should not happen if constructor validates
            core::logging::getLogger()->error("Invalid PriceField requested in get_price_value");
            return 0.0; // Or throw?
    }
}

bool PriceCondition::evaluate(const MarketDataSnapshot& snapshot) const {
    if (!snapshot.current_candle) {
        // Cannot evaluate if there's no current candle data
        // Log this? Or assume caller handles valid snapshots? Let's log trace.
         core::logging::getLogger()->trace("PriceCondition evaluate failed: Snapshot has no current candle.");
        return false;
    }

    const core::Candle& candle = *snapshot.current_candle;
    double lhs_value = get_price_value(candle, field1_);
    double rhs_value = compare_to_value_ ? value_ : get_price_value(candle, field2_);

    switch (op_) {
        case ComparisonOp::GT:  return lhs_value > rhs_value;
        case ComparisonOp::LT:  return lhs_value < rhs_value;
        case ComparisonOp::GTE: return lhs_value >= rhs_value;
        case ComparisonOp::LTE: return lhs_value <= rhs_value;
        case ComparisonOp::EQ:
            // Use tolerance for floating point equality
            return std::fabs(lhs_value - rhs_value) < 1e-9; // Adjust tolerance if needed
        default:
             core::logging::getLogger()->error("Invalid ComparisonOp in PriceCondition::evaluate");
            return false;
    }
}

 // --- Helper string conversions for describe() ---
 std::string PriceCondition::field_to_string(PriceField field) const {
      switch (field) {
         case PriceField::Open:  return "Open";
         case PriceField::High:  return "High";
         case PriceField::Low:   return "Low";
         case PriceField::Close: return "Close";
         default: return "InvalidField";
     }
 }

  std::string PriceCondition::op_to_string(ComparisonOp op) const {
     switch (op) {
         case ComparisonOp::GT:  return ">";
         case ComparisonOp::LT:  return "<";
         case ComparisonOp::GTE: return ">=";
         case ComparisonOp::LTE: return "<=";
         case ComparisonOp::EQ:  return "==";
         default: return "InvalidOp";
     }
 }
 // --- End Helper ---


std::string PriceCondition::describe() const {
    if (compare_to_value_) {
        return fmt::format("{} {} {}",
            field_to_string(field1_),
            op_to_string(op_),
            value_); // fmt should handle double formatting
    } else {
         return fmt::format("{} {} {}",
             field_to_string(field1_),
             op_to_string(op_),
             field_to_string(field2_));
    }
}

} // namespace strategy_engine
#include "price_indicator_condition.hpp"
#include "logging.hpp" // Use short path
#include "spdlog/fmt/bundled/core.h" // Use direct path for safety
#include <cmath>     // For std::fabs

namespace strategy_engine {

PriceIndicatorCondition::PriceIndicatorCondition(PriceField price_field, ComparisonOp op, std::string indicator_name)
    : price_field_(price_field), op_(op), indicator_name_(std::move(indicator_name))
{
     if (indicator_name_.empty()) {
        throw std::invalid_argument("Indicator name cannot be empty for PriceIndicatorCondition.");
     }
}

// Helper to get price from candle (Duplicate from PriceCondition - refactor later!)
 double PriceIndicatorCondition::get_price_value(const core::Candle& candle, PriceField field) const {
     switch (field) {
         case PriceField::Open:  return candle.open;
         case PriceField::High:  return candle.high;
         case PriceField::Low:   return candle.low;
         case PriceField::Close: return candle.close;
         default: return 0.0; // Should not happen
     }
 }


bool PriceIndicatorCondition::evaluate(const MarketDataSnapshot& snapshot) const {
    auto logger = core::logging::getLogger();

    if (!snapshot.current_candle) {
         logger->trace("PriceIndicatorCondition evaluate failed: Snapshot has no current candle.");
         return false;
    }

    // Get Price Value
    double lhs_value = get_price_value(*snapshot.current_candle, price_field_);

    // Get Indicator Value
    auto it = snapshot.indicator_values.find(indicator_name_);
    if (it == snapshot.indicator_values.end()) {
        logger->trace("PriceIndicatorCondition evaluate failed: Indicator '{}' not found in snapshot.", indicator_name_);
        return false; // Cannot evaluate if indicator value is missing
    }
    double rhs_value = it->second;


    // Perform the comparison
     switch (op_) {
        case ComparisonOp::GT:  return lhs_value > rhs_value;
        case ComparisonOp::LT:  return lhs_value < rhs_value;
        case ComparisonOp::GTE: return lhs_value >= rhs_value;
        case ComparisonOp::LTE: return lhs_value <= rhs_value;
        case ComparisonOp::EQ:
            return std::fabs(lhs_value - rhs_value) < 1e-9;
        default:
             logger->error("Invalid ComparisonOp in PriceIndicatorCondition::evaluate");
             return false;
    }
}

  // --- Helper string conversions for describe() (Duplicates - refactor later!) ---
  std::string PriceIndicatorCondition::field_to_string(PriceField field) const {
       switch (field) {
          case PriceField::Open:  return "Open";
          case PriceField::High:  return "High";
          case PriceField::Low:   return "Low";
          case PriceField::Close: return "Close";
          default: return "InvalidField";
      }
  }

   std::string PriceIndicatorCondition::op_to_string(ComparisonOp op) const {
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

std::string PriceIndicatorCondition::describe() const {
    return fmt::format("{} {} {}",
                       field_to_string(price_field_),
                       op_to_string(op_),
                       indicator_name_);
}


} // namespace strategy_engine
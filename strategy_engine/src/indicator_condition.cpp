#include "indicator_condition.hpp"
#include "logging.hpp"    // Use short path
#include "spdlog/fmt/bundled/core.h"     // Use short path (via spdlog includes - check if direct include needed)
#include "spdlog/fmt/bundled/core.h" // Use direct path as safe fallback <<< USE THIS
#include <cmath>         // For std::fabs
#include <stdexcept>     // For std::bad_variant_access

namespace strategy_engine {

// Constructor for comparing indicator to value
IndicatorCondition::IndicatorCondition(const std::string& indicator_name1, ComparisonOp op, double value)
    : indicator_name1_(indicator_name1), op_(op), rhs_(value), compare_to_value_(true)
{
    if (indicator_name1_.empty()) {
        throw std::invalid_argument("Indicator name 1 cannot be empty.");
    }
}

// Constructor for comparing indicator to indicator
IndicatorCondition::IndicatorCondition(const std::string& indicator_name1, ComparisonOp op, const std::string& indicator_name2)
     : indicator_name1_(indicator_name1), op_(op), rhs_(indicator_name2), compare_to_value_(false)
{
     if (indicator_name1_.empty() || indicator_name2.empty()) {
         throw std::invalid_argument("Indicator names cannot be empty.");
     }
      if (indicator_name1_ == indicator_name2) {
           throw std::invalid_argument("Cannot compare an indicator to itself in IndicatorCondition.");
      }
}


bool IndicatorCondition::evaluate(const MarketDataSnapshot& snapshot) const {
    auto logger = core::logging::getLogger();

    // Find the value of the first indicator in the snapshot
    auto it1 = snapshot.indicator_values.find(indicator_name1_);
    if (it1 == snapshot.indicator_values.end()) {
        logger->trace("IndicatorCondition evaluate failed: LHS indicator '{}' not found in snapshot.", indicator_name1_);
        return false; // Cannot evaluate if indicator value is missing
    }
    double lhs_value = it1->second;

    double rhs_value = 0.0;
    if (compare_to_value_) {
        // Get the value from the variant
        try {
             rhs_value = std::get<double>(rhs_);
        } catch (const std::bad_variant_access& ex) {
             logger->error("Internal error: Variant access failed in IndicatorCondition (value). {}", ex.what());
             return false; // Should not happen
        }
    } else {
        // Get the name of the second indicator and find its value
         std::string indicator_name2;
         try {
             indicator_name2 = std::get<std::string>(rhs_);
         } catch (const std::bad_variant_access& ex) {
             logger->error("Internal error: Variant access failed in IndicatorCondition (name). {}", ex.what());
             return false; // Should not happen
         }

        auto it2 = snapshot.indicator_values.find(indicator_name2);
        if (it2 == snapshot.indicator_values.end()) {
            logger->trace("IndicatorCondition evaluate failed: RHS indicator '{}' not found in snapshot.", indicator_name2);
            return false; // Cannot evaluate if second indicator value is missing
        }
        rhs_value = it2->second;
    }

    // Perform the comparison
     switch (op_) {
        case ComparisonOp::GT:  return lhs_value > rhs_value;
        case ComparisonOp::LT:  return lhs_value < rhs_value;
        case ComparisonOp::GTE: return lhs_value >= rhs_value;
        case ComparisonOp::LTE: return lhs_value <= rhs_value;
        case ComparisonOp::EQ:
            // Use tolerance for floating point equality
            return std::fabs(lhs_value - rhs_value) < 1e-9; // Adjust tolerance if needed
        default:
             logger->error("Invalid ComparisonOp in IndicatorCondition::evaluate");
            return false;
    }

    return false;
}

 // --- Helper string conversion for describe() ---
 // TODO: Refactor this into a common utility later
 std::string IndicatorCondition::op_to_string(ComparisonOp op) const {
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

std::string IndicatorCondition::describe() const {
    if (compare_to_value_) {
         try {
            return fmt::format("{} {} {}", indicator_name1_, op_to_string(op_), std::get<double>(rhs_));
         } catch (const std::bad_variant_access&) { return "Error describing condition"; }
    } else {
         try {
            return fmt::format("{} {} {}", indicator_name1_, op_to_string(op_), std::get<std::string>(rhs_));
          } catch (const std::bad_variant_access&) { return "Error describing condition"; }
    }
}


} // namespace strategy_engine
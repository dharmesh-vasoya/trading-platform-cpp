#include "rule.hpp"
#include "logging.hpp" // Use short path
#include "spdlog/fmt/bundled/core.h" // Use direct path for safety
#include <stdexcept> // For std::invalid_argument

namespace strategy_engine {

Rule::Rule(std::string rule_name,
           std::unique_ptr<ICondition> condition,
           core::SignalAction action_on_true)
    : name_(std::move(rule_name)), // Use move for efficiency
      condition_(std::move(condition)), // Take ownership
      action_(action_on_true)
{
    if (name_.empty()) {
         throw std::invalid_argument("Rule name cannot be empty.");
    }
    if (!condition_) {
         // We must have a valid condition
         throw std::invalid_argument(fmt::format("Condition cannot be null for Rule '{}'.", name_));
    }
    if (action_ == core::SignalAction::None) {
         // Rule must have a specific action associated if condition is true
          throw std::invalid_argument(fmt::format("Action cannot be 'None' for Rule '{}'.", name_));
    }
}

core::SignalAction Rule::evaluate(const MarketDataSnapshot& snapshot) const {
    // Condition pointer was checked in constructor, should be valid
    bool condition_result = condition_->evaluate(snapshot);

    core::logging::getLogger()->trace("Rule '{}' evaluated condition '{}' -> {}",
                                    name_, condition_->describe(), condition_result);

    if (condition_result) {
        return action_; // Condition is true, return the rule's action
    } else {
        return core::SignalAction::None; // Condition is false, no action triggered by this rule
    }
}

// Helper to convert SignalAction enum to string (consider moving to core/utils?)
inline std::string action_to_string(core::SignalAction action) {
     switch(action) {
         case core::SignalAction::None:       return "None";
         case core::SignalAction::EnterLong:  return "EnterLong";
         case core::SignalAction::ExitLong:   return "ExitLong";
         case core::SignalAction::EnterShort: return "EnterShort";
         case core::SignalAction::ExitShort:  return "ExitShort";
         default:                             return "UnknownAction";
     }
 }

std::string Rule::describe() const {
    return fmt::format("Rule('{}'): IF ({}) THEN {}",
                       name_,
                       condition_ ? condition_->describe() : "NullCondition",
                       action_to_string(action_));
}


} // namespace strategy_engine
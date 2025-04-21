#include "or_condition.hpp"
#include <sstream> // For describe()

namespace strategy_engine {

OrCondition::OrCondition(std::vector<std::unique_ptr<ICondition>> conditions)
    : conditions_(std::move(conditions)) // Take ownership via move
{
     if (conditions_.empty()) {
         // Or should an empty OR condition be false? Let's forbid it.
         throw std::invalid_argument("OrCondition must receive at least one condition.");
     }
}

bool OrCondition::evaluate(const MarketDataSnapshot& snapshot) const {
    for (const auto& condition : conditions_) {
        if (condition && condition->evaluate(snapshot)) {
            return true; // If any condition is non-null and true, the OR is true
        }
    }
    return false; // All conditions evaluated to false (or were null)
}

 std::string OrCondition::describe() const {
     if (conditions_.empty()) {
         return "(Empty OR)";
     }
    std::stringstream ss;
    ss << "(";
    for (size_t i = 0; i < conditions_.size(); ++i) {
        ss << (conditions_[i] ? conditions_[i]->describe() : "NullCondition");
        if (i < conditions_.size() - 1) {
            ss << " OR ";
        }
    }
    ss << ")";
    return ss.str();
}


} // namespace strategy_engine
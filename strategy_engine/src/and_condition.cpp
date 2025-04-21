#include "and_condition.hpp"
#include <sstream> // For describe()

namespace strategy_engine {

AndCondition::AndCondition(std::vector<std::unique_ptr<ICondition>> conditions)
    : conditions_(std::move(conditions)) // Take ownership via move
{
    if (conditions_.empty()) {
        // Or should an empty AND condition be true? Let's forbid it for now.
         throw std::invalid_argument("AndCondition must receive at least one condition.");
    }
}

bool AndCondition::evaluate(const MarketDataSnapshot& snapshot) const {
    for (const auto& condition : conditions_) {
        if (!condition || !condition->evaluate(snapshot)) {
            return false; // If any condition is null or false, the AND is false
        }
    }
    return true; // All conditions evaluated to true
}

std::string AndCondition::describe() const {
    if (conditions_.empty()) {
         return "(Empty AND)"; // Should not happen with constructor check
    }
    std::stringstream ss;
    ss << "(";
    for (size_t i = 0; i < conditions_.size(); ++i) {
        ss << (conditions_[i] ? conditions_[i]->describe() : "NullCondition");
        if (i < conditions_.size() - 1) {
            ss << " AND ";
        }
    }
    ss << ")";
    return ss.str();
}

} // namespace strategy_engine
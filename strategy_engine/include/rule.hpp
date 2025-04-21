#pragma once

#include "interfaces.hpp" // Includes ICondition, IRule, SignalAction etc.
#include <string>
#include <memory> // For std::unique_ptr

namespace strategy_engine {

    // --- Rule Class ---
    // Represents a single trading rule (e.g., an entry rule or an exit rule).
    // It holds a condition and the action to take if the condition evaluates to true.
    class Rule : public IRule {
    public:
        // Constructor: Takes a name, ownership of a condition object, and the action.
        Rule(std::string rule_name,
             std::unique_ptr<ICondition> condition,
             core::SignalAction action_on_true);

        virtual ~Rule() override = default;

        // IRule interface implementation
        core::SignalAction evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

        // Getter for the rule name
        std::string getName() const override { return name_; }

    private:
        std::string name_;
        std::unique_ptr<ICondition> condition_; // The condition to evaluate
        core::SignalAction action_;             // Action to return if condition is true
    };

} // namespace strategy_engine
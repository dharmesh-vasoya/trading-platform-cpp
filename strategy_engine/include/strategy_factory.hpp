#pragma once

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <set> 
#include <nlohmann/json.hpp> // Include JSON library header

// Forward declare or include interfaces
#include "interfaces.hpp"

namespace strategy_engine {

    using json = nlohmann::json; // Alias for convenience

    class StrategyFactory {
    public:
        // Static method to create a strategy from a JSON config object
        static std::unique_ptr<IStrategy> createStrategy(const json& config);

    private:
        // Private helper methods for parsing components (can be implemented later)
        static std::unique_ptr<ICondition> parseCondition(const json& condition_config);
        static std::unique_ptr<IRule> parseRule(const json& rule_config);
        // Helper to get required indicator names from conditions (recursive)
        static void collectIndicatorNames(const json& condition_config, std::set<std::string>& names); // Changed vector to set
    };

} // namespace strategy_engine
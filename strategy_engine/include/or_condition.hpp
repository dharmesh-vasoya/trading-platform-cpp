#pragma once

#include "interfaces.hpp"
#include <vector>
#include <memory> // For std::unique_ptr
#include <string>
#include <numeric> // For std::accumulate (optional for describe)


namespace strategy_engine {

    // --- OrCondition Class ---
    // Evaluates to true if ANY contained conditions evaluate to true.
    class OrCondition : public ICondition {
    public:
        // Constructor takes ownership of a vector of conditions
        explicit OrCondition(std::vector<std::unique_ptr<ICondition>> conditions);

        virtual ~OrCondition() override = default;

        // ICondition interface implementation
        bool evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

    private:
        std::vector<std::unique_ptr<ICondition>> conditions_;
    };

} // namespace strategy_engine
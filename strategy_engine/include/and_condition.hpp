#pragma once

#include "interfaces.hpp"
#include <vector>
#include <memory> // For std::unique_ptr
#include <string>
#include <numeric> // For std::accumulate (optional for describe)

namespace strategy_engine {

    // --- AndCondition Class ---
    // Evaluates to true only if ALL contained conditions evaluate to true.
    class AndCondition : public ICondition {
    public:
        // Constructor takes ownership of a vector of conditions
        explicit AndCondition(std::vector<std::unique_ptr<ICondition>> conditions);

        virtual ~AndCondition() override = default;

        // ICondition interface implementation
        bool evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

    private:
        std::vector<std::unique_ptr<ICondition>> conditions_;
    };

} // namespace strategy_engine
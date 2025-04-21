#pragma once

#include "interfaces.hpp"
#include "common_types.hpp" // For ComparisonOp (though maybe not needed)
#include <string>
#include <stdexcept>

namespace strategy_engine {

    // Enum for Cross Type
    enum class CrossType {
        CrossesAbove,
        CrossesBelow
    };

    // --- IndicatorCrossCondition Class ---
    // Checks if indicator1 crossed above/below indicator2 in the current step.
    class IndicatorCrossCondition : public ICondition {
    public:
        // Constructor: e.g., IndicatorCrossCondition("SMA(10)", CrossType::CrossesAbove, "SMA(20)")
        IndicatorCrossCondition(std::string indicator1_name,
                                CrossType cross_type,
                                std::string indicator2_name);

        virtual ~IndicatorCrossCondition() override = default;

        // ICondition interface implementation
        bool evaluate(const MarketDataSnapshot& snapshot) const override;
        std::string describe() const override;

    private:
        std::string indicator1_name_;
        CrossType cross_type_;
        std::string indicator2_name_;

        // Helper
         std::string crosstype_to_string(CrossType type) const;
    };

} // namespace strategy_engine
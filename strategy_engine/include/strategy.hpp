#pragma once

#include "interfaces.hpp" // Includes IRule, IStrategy, MarketDataSnapshot etc.
#include "datatypes.hpp"  // Includes PositionState, SignalAction etc. (use short path)
#include "common_types.hpp"
#include <string>
#include <vector>
#include <memory>      // For std::unique_ptr
#include <map>         // For parameters

namespace strategy_engine {

    // --- Strategy Class ---
    // Concrete implementation of IStrategy
    class Strategy : public IStrategy {
    public:
        // Constructor
        Strategy(
            std::string name,
            // std::map<std::string, double> params, // TODO: Add params later if needed
            std::vector<std::string> required_instruments,
            std::vector<std::string> required_timeframes,
            std::vector<std::string> required_indicator_names,
            std::vector<std::unique_ptr<IRule>> entry_rules,
            std::vector<std::unique_ptr<IRule>> exit_rules,
            SizingMethod sizing_method,
            double sizing_value,
            bool is_sizing_value_percentage // Flag for CapitalBased method
        );

        virtual ~Strategy() override = default;

        // IStrategy interface implementation
        std::string getName() const override;
        const std::vector<std::string>& getRequiredInstruments() const override;
        const std::vector<std::string>& getRequiredTimeframes() const override;
        const std::vector<std::string>& getRequiredIndicatorNames() const override;
        core::SignalAction evaluate(const MarketDataSnapshot& snapshot) override;

        SizingMethod getSizingMethod() const override { return sizing_method_; }
        double getSizingValue() const override { return sizing_value_; }
        bool isSizingValuePercentage() const override { return is_sizing_value_percentage_; }
        
        // Get current position state (needed for backtester/execution)
        core::PositionState getCurrentPosition() const; 

    private:
        std::string name_;
        // std::map<std::string, double> params_; // Parameters like SMA period etc.
        std::vector<std::string> required_instruments_;
        std::vector<std::string> required_timeframes_;
        std::vector<std::string> required_indicator_names_;
        std::vector<std::unique_ptr<IRule>> entry_rules_;
        std::vector<std::unique_ptr<IRule>> exit_rules_;

        core::PositionState current_position_ = core::PositionState::None; // Track current state
        SizingMethod sizing_method_;
        double sizing_value_;
        bool is_sizing_value_percentage_;
    };

} // namespace strategy_engine
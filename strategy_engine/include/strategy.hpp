#pragma once

#include "interfaces.hpp" // Includes IRule, IStrategy, MarketDataSnapshot etc.
#include "datatypes.hpp"  // Includes PositionState, SignalAction etc. (use short path)
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
            std::vector<std::unique_ptr<IRule>> exit_rules
        );

        virtual ~Strategy() override = default;

        // IStrategy interface implementation
        std::string getName() const override;
        const std::vector<std::string>& getRequiredInstruments() const override;
        const std::vector<std::string>& getRequiredTimeframes() const override;
        const std::vector<std::string>& getRequiredIndicatorNames() const override;
        core::SignalAction evaluate(const MarketDataSnapshot& snapshot) override;

        // Get current position state (needed for backtester/execution)
        core::PositionState getCurrentPosition() const { return current_position_; }

    private:
        std::string name_;
        // std::map<std::string, double> params_; // Parameters like SMA period etc.
        std::vector<std::string> required_instruments_;
        std::vector<std::string> required_timeframes_;
        std::vector<std::string> required_indicator_names_;
        std::vector<std::unique_ptr<IRule>> entry_rules_;
        std::vector<std::unique_ptr<IRule>> exit_rules_;

        core::PositionState current_position_ = core::PositionState::None; // Track current state
    };

} // namespace strategy_engine
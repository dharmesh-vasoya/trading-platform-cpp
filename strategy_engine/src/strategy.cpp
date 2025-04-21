#include "strategy.hpp"
#include "logging.hpp" // Use short path
#include <stdexcept>  // For std::invalid_argument

namespace strategy_engine {

Strategy::Strategy(
    std::string name,
    // std::map<std::string, double> params,
    std::vector<std::string> required_instruments,
    std::vector<std::string> required_timeframes,
    std::vector<std::string> required_indicator_names,
    std::vector<std::unique_ptr<IRule>> entry_rules,
    std::vector<std::unique_ptr<IRule>> exit_rules
) : name_(std::move(name)),
    // params_(std::move(params)),
    required_instruments_(std::move(required_instruments)),
    required_timeframes_(std::move(required_timeframes)),
    required_indicator_names_(std::move(required_indicator_names)),
    entry_rules_(std::move(entry_rules)),
    exit_rules_(std::move(exit_rules)),
    current_position_(core::PositionState::None) // Start flat
{
    if (name_.empty()) throw std::invalid_argument("Strategy name cannot be empty.");
    if (required_instruments_.empty()) throw std::invalid_argument("Strategy must require at least one instrument.");
    if (required_timeframes_.empty()) throw std::invalid_argument("Strategy must require at least one timeframe.");
    if (entry_rules_.empty()) throw std::invalid_argument("Strategy must have at least one entry rule.");
    // Exit rules might be optional (e.g., fixed stop/profit target handled elsewhere)
    // if (exit_rules_.empty()) throw std::invalid_argument("Strategy must have at least one exit rule.");

    core::logging::getLogger()->debug("Strategy '{}' created.", name_);
}


std::string Strategy::getName() const { return name_; }
const std::vector<std::string>& Strategy::getRequiredInstruments() const { return required_instruments_; }
const std::vector<std::string>& Strategy::getRequiredTimeframes() const { return required_timeframes_; }
const std::vector<std::string>& Strategy::getRequiredIndicatorNames() const { return required_indicator_names_; }


core::SignalAction Strategy::evaluate(const MarketDataSnapshot& snapshot) {
    auto logger = core::logging::getLogger();
    logger->trace("Evaluating strategy '{}', current position: {}", name_, static_cast<int>(current_position_));

    core::SignalAction resulting_action = core::SignalAction::None;

    // --- Evaluate Rules based on Current Position ---
    if (current_position_ == core::PositionState::None) {
        // Currently Flat: Check ENTRY rules
        logger->trace("Checking entry rules for strategy '{}'", name_);
        for (const auto& rule : entry_rules_) {
            if (!rule) continue; // Skip null rules
            core::SignalAction action = rule->evaluate(snapshot);
            if (action == core::SignalAction::EnterLong || action == core::SignalAction::EnterShort) {
                logger->debug("Strategy '{}': Entry rule '{}' triggered -> {}", name_, rule->getName(), static_cast<int>(action));
                resulting_action = action;
                break; // Take the first entry signal
            }
        }
    } else {
        // Currently Long or Short: Check EXIT rules
         logger->trace("Checking exit rules for strategy '{}'", name_);
         for (const auto& rule : exit_rules_) {
             if (!rule) continue; // Skip null rules
             core::SignalAction action = rule->evaluate(snapshot);
             // Check if the exit action matches the current position
             if ((current_position_ == core::PositionState::Long && action == core::SignalAction::ExitLong) ||
                 (current_position_ == core::PositionState::Short && action == core::SignalAction::ExitShort))
             {
                  logger->debug("Strategy '{}': Exit rule '{}' triggered -> {}", name_, rule->getName(), static_cast<int>(action));
                  resulting_action = action;
                  break; // Take the first valid exit signal
             }
         }
    }

    // --- Update Position State based on Action ---
    // Note: This simple state machine assumes immediate fills.
    // A real backtester would handle pending orders.
    switch (resulting_action) {
        case core::SignalAction::EnterLong:
            current_position_ = core::PositionState::Long;
            break;
        case core::SignalAction::EnterShort:
             current_position_ = core::PositionState::Short;
             break;
        case core::SignalAction::ExitLong:
        case core::SignalAction::ExitShort:
            current_position_ = core::PositionState::None;
             break;
        case core::SignalAction::None:
            // No change in position
            break;
    }

    logger->trace("Strategy '{}' evaluation complete. Action: {}, New Position: {}",
                  name_, static_cast<int>(resulting_action), static_cast<int>(current_position_));

    return resulting_action;
}


} // namespace strategy_engine
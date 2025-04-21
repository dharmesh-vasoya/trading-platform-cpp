#include "indicator_cross_condition.hpp"
#include "logging.hpp"    // Use short path
#include "spdlog/fmt/bundled/core.h" // Use direct path for safety

namespace strategy_engine {

IndicatorCrossCondition::IndicatorCrossCondition(std::string indicator1_name,
                                               CrossType cross_type,
                                               std::string indicator2_name)
    : indicator1_name_(std::move(indicator1_name)),
      cross_type_(cross_type),
      indicator2_name_(std::move(indicator2_name))
{
     if (indicator1_name_.empty() || indicator2_name_.empty()) {
         throw std::invalid_argument("Indicator names cannot be empty for IndicatorCrossCondition.");
     }
      if (indicator1_name_ == indicator2_name_) {
           throw std::invalid_argument("Cannot check cross condition for the same indicator.");
      }
}

bool IndicatorCrossCondition::evaluate(const MarketDataSnapshot& snapshot) const {
    auto logger = core::logging::getLogger();

    // Get current values
    auto it1_now = snapshot.indicator_values.find(indicator1_name_);
    auto it2_now = snapshot.indicator_values.find(indicator2_name_);

    // Get previous values
    auto it1_prev = snapshot.indicator_values_prev.find(indicator1_name_);
    auto it2_prev = snapshot.indicator_values_prev.find(indicator2_name_);

    // Check if all four values are available
    if (it1_now == snapshot.indicator_values.end() ||
        it2_now == snapshot.indicator_values.end() ||
        it1_prev == snapshot.indicator_values_prev.end() ||
        it2_prev == snapshot.indicator_values_prev.end())
    {
         logger->trace("IndicatorCrossCondition evaluate failed: Missing current or previous indicator values ('{}', '{}').",
                       indicator1_name_, indicator2_name_);
         return false;
    }

    double val1_now = it1_now->second;
    double val2_now = it2_now->second;
    double val1_prev = it1_prev->second;
    double val2_prev = it2_prev->second;

    // Check for crossover type
    if (cross_type_ == CrossType::CrossesAbove) {
        // Was below or equal previously, AND is above now
        return (val1_prev <= val2_prev) && (val1_now > val2_now);
    } else { // CrossesBelow
         // Was above or equal previously, AND is below now
         return (val1_prev >= val2_prev) && (val1_now < val2_now);
    }
}

 std::string IndicatorCrossCondition::crosstype_to_string(CrossType type) const {
     return (type == CrossType::CrossesAbove) ? "CrossesAbove" : "CrossesBelow";
 }

std::string IndicatorCrossCondition::describe() const {
    return fmt::format("{} {} {}",
                       indicator1_name_,
                       crosstype_to_string(cross_type_),
                       indicator2_name_);
}

} // namespace strategy_engine
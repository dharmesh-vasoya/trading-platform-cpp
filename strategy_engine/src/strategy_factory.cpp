#include "strategy_factory.hpp"
#include "strategy.hpp"           // Concrete Strategy class
#include "rule.hpp"               // Concrete Rule class
#include "price_condition.hpp"    // Concrete Condition classes...
#include "indicator_condition.hpp"
#include "and_condition.hpp"
#include "or_condition.hpp"
#include "logging.hpp"            // Use short path
#include "exceptions.hpp"
#include "common_types.hpp"
#include "spdlog/fmt/bundled/core.h" // Use direct path for safety
#include "price_indicator_condition.hpp"
#include "indicator_cross_condition.hpp"
#include <stdexcept>              // For std::invalid_argument
#include <vector>
#include <string>
#include <memory>
#include <set> // To help collect unique indicator names


namespace strategy_engine {

    using json = nlohmann::json; // Alias

    namespace { // Use anonymous namespace for file-local helpers

        core::SignalAction stringToAction(const std::string& action_str) {
            if (action_str == "EnterLong") return core::SignalAction::EnterLong;
            if (action_str == "ExitLong") return core::SignalAction::ExitLong;
            if (action_str == "EnterShort") return core::SignalAction::EnterShort;
            if (action_str == "ExitShort") return core::SignalAction::ExitShort;
            if (action_str == "None") return core::SignalAction::None; // Should generally not be used in rules
            throw std::invalid_argument("Unknown signal action string: " + action_str);
        }
    
        ComparisonOp stringToCompOp(const std::string& op_str) {
            if (op_str == ">" || op_str == "GT") return ComparisonOp::GT;
            if (op_str == "<" || op_str == "LT") return ComparisonOp::LT;
            if (op_str == ">=" || op_str == "GTE") return ComparisonOp::GTE;
            if (op_str == "<=" || op_str == "LTE") return ComparisonOp::LTE;
            if (op_str == "==" || op_str == "EQ") return ComparisonOp::EQ;
            throw std::invalid_argument("Unknown comparison operator string: " + op_str);
        }
    
        PriceField stringToPriceField(const std::string& field_str) {
            std::string lower_str = field_str;
            // Convert to lowercase for case-insensitive matching
            std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
            if (lower_str == "open") return PriceField::Open;
            if (lower_str == "high") return PriceField::High;
            if (lower_str == "low") return PriceField::Low;
            if (lower_str == "close") return PriceField::Close;
            throw std::invalid_argument("Unknown price field string: " + field_str);
        }

        SizingMethod stringToSizingMethod(const std::string& method_str) {
            std::string lower_str = method_str;
            std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
            if (lower_str == "quantity") return SizingMethod::Quantity;
            if (lower_str == "capitalbased") return SizingMethod::CapitalBased;
            throw std::invalid_argument("Unknown position sizing method: " + method_str);
       }
    
    } // end anonymous namespace
    
    // --- Recursive Helper to Parse Conditions ---
    std::unique_ptr<ICondition> StrategyFactory::parseCondition(const json& config) {
        if (!config.is_object() || !config.contains("type") || !config["type"].is_string()) {
            throw std::invalid_argument("Condition config must be an object with a 'type' (string).");
        }
        std::string type = config["type"].get<std::string>();
        auto logger = core::logging::getLogger();
        logger->trace("Parsing condition of type: {}", type);
    
        try { // Add try-catch around parsing logic for better error context
            if (type == "Price") {
                if (!config.contains("field1") || !config["field1"].is_string() ||
                    !config.contains("op") || !config["op"].is_string()) {
                    throw std::invalid_argument("Price condition requires 'field1' (string) and 'op' (string).");
                }
                PriceField field1 = stringToPriceField(config["field1"].get<std::string>());
                ComparisonOp op = stringToCompOp(config["op"].get<std::string>());
    
                if (config.contains("value") && config["value"].is_number()) {
                    double value = config["value"].get<double>();
                    return std::make_unique<PriceCondition>(field1, op, value);
                } else if (config.contains("field2") && config["field2"].is_string()) {
                    PriceField field2 = stringToPriceField(config["field2"].get<std::string>());
                    return std::make_unique<PriceCondition>(field1, op, field2);
                } else {
                    throw std::invalid_argument("Price condition requires 'value' (number) or 'field2' (string).");
                }
            } else if (type == "Indicator") {
                if (!config.contains("indicator1") || !config["indicator1"].is_string() ||
                    !config.contains("op") || !config["op"].is_string()) {
                    throw std::invalid_argument("Indicator condition requires 'indicator1' (string) and 'op' (string).");
                }
                ComparisonOp op = stringToCompOp(config["op"].get<std::string>());
                std::string indicator1 = config["indicator1"].get<std::string>();
    
                if (config.contains("value") && config["value"].is_number()) {
                    double value = config["value"].get<double>();
                    return std::make_unique<IndicatorCondition>(indicator1, op, value);
                } else if (config.contains("indicator2") && config["indicator2"].is_string()) {
                    std::string indicator2 = config["indicator2"].get<std::string>();
                    return std::make_unique<IndicatorCondition>(indicator1, op, indicator2);
                } else {
                    throw std::invalid_argument("Indicator condition requires 'value' (number) or 'indicator2' (string).");
                }
            } else if (type == "PriceIndicator") {
                if (!config.contains("price_field") || !config["price_field"].is_string() ||
                    !config.contains("op") || !config["op"].is_string() ||
                    !config.contains("indicator") || !config["indicator"].is_string()) {
                    throw std::invalid_argument("PriceIndicator condition requires 'price_field'(string), 'op'(string), 'indicator'(string).");
                }
                PriceField field = stringToPriceField(config["price_field"].get<std::string>());
                ComparisonOp op = stringToCompOp(config["op"].get<std::string>());
                std::string indicator_name = config["indicator"].get<std::string>();
                return std::make_unique<PriceIndicatorCondition>(field, op, indicator_name);
           } else if (type == "CrossesAbove" || type == "CrossesBelow") {
                if (!config.contains("indicator1") || !config["indicator1"].is_string() ||
                    !config.contains("indicator2") || !config["indicator2"].is_string()) {
                    throw std::invalid_argument(fmt::format("{} condition requires 'indicator1'(string) and 'indicator2'(string).", type));
                }
                std::string indicator1 = config["indicator1"].get<std::string>();
                std::string indicator2 = config["indicator2"].get<std::string>();
                CrossType cross_type = (type == "CrossesAbove") ? CrossType::CrossesAbove : CrossType::CrossesBelow;
                return std::make_unique<IndicatorCrossCondition>(indicator1, cross_type, indicator2);
            } else if (type == "AND" || type == "OR") {
                if (!config.contains("conditions") || !config["conditions"].is_array() || config["conditions"].empty()) {
                    throw std::invalid_argument(fmt::format("{} condition requires 'conditions' (non-empty array).", type));
                }
                std::vector<std::unique_ptr<ICondition>> sub_conditions;
                sub_conditions.reserve(config["conditions"].size()); // Optimization
                for (const auto& sub_conf : config["conditions"]) {
                    sub_conditions.push_back(parseCondition(sub_conf)); // Recursive call
                    if (!sub_conditions.back()) {
                        throw std::runtime_error(fmt::format("Failed to parse sub-condition within {} condition.", type));
                    }
                }
                if (type == "AND") {
                    return std::make_unique<AndCondition>(std::move(sub_conditions));
                } else { // OR
                    return std::make_unique<OrCondition>(std::move(sub_conditions));
                }
            }
            // TODO: Add parsing for other condition types like "CrossesAbove", "CrossesBelow"
            else {
                throw std::invalid_argument(fmt::format("Unknown condition type '{}' in config.", type));
            }
        } catch (const json::exception& e) {
             logger->error("JSON error parsing condition type '{}': {}", type, e.what());
             throw std::invalid_argument(fmt::format("Invalid JSON structure for condition type '{}'", type));
        } catch (const std::invalid_argument& e) {
             logger->error("Invalid argument parsing condition type '{}': {}", type, e.what());
             throw; // Re-throw invalid_argument
        }
    }

    // --- Helper to Parse Rules ---
    std::unique_ptr<IRule> StrategyFactory::parseRule(const json& config) {
        if (!config.is_object() ||
            !config.contains("rule_name") || !config["rule_name"].is_string() ||
            !config.contains("action") || !config["action"].is_string() ||
            !config.contains("condition") || !config["condition"].is_object())
        {
             throw std::invalid_argument("Rule config must be object with 'rule_name'(string), 'action'(string), 'condition'(object).");
        }
        std::string name = config["rule_name"].get<std::string>();
        std::string action_str = config["action"].get<std::string>();
   
        try {
            core::SignalAction action = stringToAction(action_str); // Use helper
            if (action == core::SignalAction::None) {
                throw std::invalid_argument("Rule action cannot be 'None'.");
            }
   
            auto condition = parseCondition(config["condition"]); // Delegate condition parsing
            if (!condition) {
                 // parseCondition should throw on failure, but double-check
                 throw std::runtime_error(fmt::format("Failed to parse condition for rule '{}'.", name));
            }
   
            return std::make_unique<Rule>(name, std::move(condition), action);
   
        } catch (const std::invalid_argument& e) {
             core::logging::getLogger()->error("Invalid config for rule '{}': {}", name, e.what());
             throw; // Re-throw
        }
   }
    // --- Helper to Collect Indicator Names (Recursive) ---
    void StrategyFactory::collectIndicatorNames(const json& config, std::set<std::string>& names) {
        if (!config.is_object()) return;

         if (config.contains("type") && config["type"].is_string()) {
             std::string type = config["type"].get<std::string>();

             if (type == "Indicator") {
                 if (config.contains("indicator1") && config["indicator1"].is_string()) {
                     names.insert(config["indicator1"].get<std::string>());
                 }
                 if (config.contains("indicator2") && config["indicator2"].is_string()) {
                     names.insert(config["indicator2"].get<std::string>());
                 }
             } else if (type == "PriceIndicator") {
                if (config.contains("indicator") && config["indicator"].is_string()) {
                    names.insert(config["indicator"].get<std::string>());
                }
           } else if (type == "CrossesAbove" || type == "CrossesBelow") {
            // These types compare two indicators
            if (config.contains("indicator1") && config["indicator1"].is_string()) {
               names.insert(config["indicator1"].get<std::string>());
            }
            if (config.contains("indicator2") && config["indicator2"].is_string()) {
               names.insert(config["indicator2"].get<std::string>());
            }
            // TODO: Add logic here if we later allow Price vs Indicator crosses
            } else if (type == "AND" || type == "OR") {
                 if (config.contains("conditions") && config["conditions"].is_array()) {
                     for (const auto& sub_conf : config["conditions"]) {
                         collectIndicatorNames(sub_conf, names); // Recurse
                     }
                 }
             }
             // Add cases for other indicator-using conditions (e.g., CrossesAbove/Below)
         }
     }

    // --- Main Factory Method ---
    std::unique_ptr<IStrategy> StrategyFactory::createStrategy(const json& config) {
        auto logger = core::logging::getLogger();
        logger->info("Attempting to create strategy from JSON config...");

        try {
            // --- Basic Validation ---
            if (!config.is_object()) throw std::invalid_argument("Config must be JSON object.");
            if (!config.contains("strategy_name") || !config["strategy_name"].is_string()) throw std::invalid_argument("Config missing 'strategy_name'.");
            std::string name = config["strategy_name"].get<std::string>();

            if (!config.contains("instruments") || !config["instruments"].is_array() || config["instruments"].empty()) throw std::invalid_argument("Config missing 'instruments' array.");
            std::vector<std::string> instruments = config["instruments"].get<std::vector<std::string>>();

            if (!config.contains("timeframes") || !config["timeframes"].is_array() || config["timeframes"].empty()) throw std::invalid_argument("Config missing 'timeframes' array.");
             std::vector<std::string> timeframes = config["timeframes"].get<std::vector<std::string>>();

            SizingMethod sizing_method = SizingMethod::Quantity; // Default perhaps? Or make required?
            double sizing_value = 1.0; // Default value
            bool is_percentage = false; // Default for CapitalBased

            if (config.contains("position_sizing") && config["position_sizing"].is_object()) {
                const auto& sizing_config = config["position_sizing"];
                if (!sizing_config.contains("method") || !sizing_config["method"].is_string() ||
                    !sizing_config.contains("value") || !sizing_config["value"].is_number()) {
                    throw std::invalid_argument("Strategy config 'position_sizing' object requires 'method'(string) and 'value'(number).");
                }
                sizing_method = stringToSizingMethod(sizing_config["method"].get<std::string>());
                sizing_value = sizing_config["value"].get<double>();
                if (sizing_value <= 0) throw std::invalid_argument("'position_sizing.value' must be positive.");

                if (sizing_method == SizingMethod::CapitalBased) {
                    // CapitalBased can be percentage or absolute amount
                    is_percentage = sizing_config.value("is_percentage", false); // Default to false if not specified
                    if (is_percentage && (sizing_value <= 0 || sizing_value > 100)) {
                        throw std::invalid_argument("'position_sizing.value' must be between 0 and 100 when 'is_percentage' is true.");
                    }
                } else { // Quantity method
                    // Ensure value is integer-like for quantity
                    if (std::floor(sizing_value) != sizing_value) {
                        logger->warn("Sizing method is Quantity but value '{}' is not an integer. Will use floor().", sizing_value);
                    }
                    sizing_value = std::floor(sizing_value);
                    if (sizing_value < 1) throw std::invalid_argument("'position_sizing.value' must be at least 1 for Quantity method.");
                }

        } else {
                // Default if position_sizing block is missing? Or throw error? Let's default for now.
                logger->warn("Strategy config missing 'position_sizing' object. Defaulting to Quantity=1.");
                sizing_method = SizingMethod::Quantity;
                sizing_value = 1.0;
                is_percentage = false;
                // Alternatively, make it required:
                // throw std::invalid_argument("Strategy config requires 'position_sizing' object.");
        }
             // --- Parse Rules --- // <<<--- UPDATED TO CALL HELPER ---
             if (!config.contains("entry_rules") || !config["entry_rules"].is_array()) throw std::invalid_argument("Config missing 'entry_rules' array.");
             if (!config.contains("exit_rules") || !config["exit_rules"].is_array()) throw std::invalid_argument("Config missing 'exit_rules' array.");

             std::vector<std::unique_ptr<IRule>> entry_rules;
             for (const auto& rule_conf : config["entry_rules"]) {
                 entry_rules.push_back(parseRule(rule_conf)); // Use helper
                 if (!entry_rules.back()) throw std::runtime_error("Failed to parse an entry rule.");
             }

             std::vector<std::unique_ptr<IRule>> exit_rules;
              for (const auto& rule_conf : config["exit_rules"]) {
                 exit_rules.push_back(parseRule(rule_conf)); // Use helper
                  if (!exit_rules.back()) throw std::runtime_error("Failed to parse an exit rule.");
             }


            // --- Collect Required Indicators --- // <<<--- UPDATED TO CALL HELPER ---
            // We recursively collect names from conditions instead of relying on a separate list
            std::set<std::string> indicator_name_set; // Use set to automatically handle duplicates
             for (const auto& rule_conf : config["entry_rules"]) {
                 if (rule_conf.contains("condition")) collectIndicatorNames(rule_conf["condition"], indicator_name_set);
             }
             for (const auto& rule_conf : config["exit_rules"]) {
                  if (rule_conf.contains("condition")) collectIndicatorNames(rule_conf["condition"], indicator_name_set);
             }
            // Convert set to vector
            std::vector<std::string> indicator_names(indicator_name_set.begin(), indicator_name_set.end());
            logger->debug("Collected required indicator names: {}", fmt::join(indicator_names, ", "));


            // --- Create Strategy Instance ---
            logger->info("Creating Strategy instance for '{}'", name);
            auto strategy = std::make_unique<Strategy>(
                name,
                instruments,
                timeframes,
                indicator_names,
                std::move(entry_rules),
                std::move(exit_rules),
                sizing_method,             // Pass parsed sizing info
                sizing_value,              // Pass parsed sizing info
                is_percentage              // Pass parsed sizing info
            );

            logger->info("Successfully created strategy: '{}'", name);
            return strategy;

        } catch (const json::exception& e) {
             logger->error("JSON parsing error while creating strategy: {}", e.what());
             return nullptr;
        } catch (const std::invalid_argument& e) {
            logger->error("Invalid strategy configuration: {}", e.what());
            return nullptr;
        } catch (const std::exception& e) {
             logger->error("Unexpected error creating strategy: {}", e.what());
             return nullptr;
        }
        // Should not be reached
        return nullptr;
    }

    // --- TODO Implementations ---
    // NOTE: The enum parsing (string -> enum) for PriceField and ComparisonOp
    // and SignalAction are still placeholders/warnings. These need proper implementation,
    // perhaps using maps or if/else chains inside the parseCondition/parseRule helpers.
    // Also, need to implement parsing for any new condition types (CrossesAbove etc.)


} // namespace strategy_engine
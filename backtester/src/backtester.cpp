#include "backtester.hpp"
#include "strategy_factory.hpp" // Need factory to create strategy
#include "sma_indicator.hpp"    // Need concrete indicators for createIndicator factory <<< USE SHORT PATH
// Add other indicator headers later (rsi_indicator.hpp etc.)
#include "logging.hpp"          // <<< USE SHORT PATH
#include "utils.hpp"            // <<< USE SHORT PATH
#include "exceptions.hpp"       // <<< USE SHORT PATH

#include <stdexcept>
#include <iostream>
#include <regex>       // For parsing indicator names
#include <string>      // For std::stoi
#include <utility>     // For std::pair

namespace backtester {

    Backtester::Backtester(data::DatabaseManager& db_manager, double initial_capital)
        : db_manager_(db_manager), initial_capital_(initial_capital)
    {
        portfolio_ = std::make_unique<Portfolio>(initial_capital_);
        core::logging::getLogger()->debug("Backtester initialized with capital: {}", initial_capital_);
    }

    bool Backtester::run(const json& strategy_config,
        const std::string& start_date,
        const std::string& end_date)
    {
    auto logger = core::logging::getLogger();
    logger->info("========================================================");
    logger->info("Starting Backtest Run");
    logger->info("========================================================");
    logger->info("Strategy Config: {}", strategy_config.dump(2)); // Log loaded config
    logger->info("Period: {} to {}", start_date, end_date);

    // Reset portfolio for new run
    portfolio_ = std::make_unique<Portfolio>(initial_capital_);

    try {
    // 1. Load Strategy
    strategy_ = strategy_engine::StrategyFactory::createStrategy(strategy_config);
    if (!strategy_) {
    logger->error("Failed to load strategy from config.");
    return false;
    }
    logger->info("Strategy '{}' loaded successfully.", strategy_->getName());

    // Ensure DB is connected before loading data
    if (!db_manager_.isConnected()) {
    logger->info("Connecting to DB for backtest data...");
    if (!db_manager_.connect()) {
        logger->error("Failed to connect to DB for backtest.");
        return false;
    }
    }

    // 2. Load Data
    if (!loadData(start_date, end_date)) {
    logger->error("Failed to load required data for backtest period.");
    // Disconnect DB if we connected it
    // db_manager_.disconnect(); // Or let caller manage connection
    return false;
    }

    // 3. Create & Calculate Indicators
    if (!createAndCalculateIndicators()) {
    logger->error("Failed to create/calculate required indicators.");
    // db_manager_.disconnect();
    return false;
    }

    logger->info("Starting event loop...");
    runEventLoop();
    logger->info("Event loop finished.");


    // 5. Calculate Metrics (Still a stub)
    calculateMetrics();

    logger->info("========================================================");
    logger->info("Backtest Run Completed for Strategy '{}'", strategy_->getName());
    logger->info("========================================================");
    // Optional: Log final equity or key metrics here
    return true; // Indicate run finished (though logic is incomplete)

    } catch (const std::exception& e) {
    logger->critical("Exception during backtest run: {}", e.what());
    // db_manager_.disconnect(); // Ensure disconnect on error?
    return false;
    }
    // Consider adding disconnect in finally/destructor if needed
    }

    bool Backtester::loadData(const std::string& start_date, const std::string& end_date) {
        auto logger = core::logging::getLogger();
        logger->info("Loading historical data for backtest...");
    
        if (!strategy_) {
            logger->error("Cannot load data: Strategy not loaded yet.");
            return false;
        }
        if (!db_manager_.isConnected()) {
             logger->error("Cannot load data: Database not connected.");
             // Should the backtester manage the connection? Or assume it's connected?
             // For now, assume main connects/disconnects. If fails here, return false.
             return false;
        }
    
        const auto& instruments = strategy_->getRequiredInstruments();
        const auto& timeframes = strategy_->getRequiredTimeframes();
    
        // --- Simplified Logic: Use first instrument/timeframe ---
        // TODO: Enhance later to handle multi-instrument / multi-timeframe strategies
        if(instruments.empty() || timeframes.empty()) {
             logger->error("Strategy requires no instruments or timeframes.");
             return false;
        }
        primary_instrument_key_ = instruments[0]; // Store the key we loaded
        std::string primary_timeframe = timeframes[0]; // Assumes this matches DB interval string ('day')
        logger->info("Primary data target: {} ({})", primary_instrument_key_, primary_timeframe);
        // --- End Simplification ---
    
        try {
            // Convert start/end dates (YYYY-MM-DD) to Timestamps for query
            // Assume we query from start of start_date to *end* of end_date
            // Adjust time/timezone details to match database storage (+05:30)
            core::Timestamp start_ts = core::utils::stringToTimestamp(start_date + "T00:00:00+05:30");
            core::Timestamp end_ts = core::utils::stringToTimestamp(end_date + "T23:59:59+05:30");
    
            logger->info("Querying database for primary data...");
            primary_data_ = db_manager_.queryCandles(primary_instrument_key_, primary_timeframe, start_ts, end_ts);
            logger->info("Loaded {} primary data points.", primary_data_.size());
    
            if (primary_data_.empty()) {
                 logger->error("No primary historical data found for the specified range.");
                 return false; // Cannot run backtest without data
            }
            return true; // Data loaded successfully
    
        } catch (const std::exception& e) {
             logger->error("Error loading data: {}", e.what());
             return false;
        }
    }

    // Helper to parse indicator name and period (e.g., "SMA(10)")
    // Returns {name, period} or {name, -1} if no period found
    // TODO: Move this to a common indicator utility?
    inline std::pair<std::string, int> parseIndicatorString(const std::string& indicator_str) {
         static const std::regex indicator_regex(R"((.+)\((\d+)\))"); // e.g., SMA(10)
         std::smatch match;
         if (std::regex_match(indicator_str, match, indicator_regex) && match.size() == 3) {
             try {
                 // Convert base name to uppercase for consistent matching? Optional.
                 // std::string base_name = match[1].str();
                 // std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);
                 // return {base_name, std::stoi(match[2].str())};
                  return {match[1].str(), std::stoi(match[2].str())};
             } catch (...) { /* ignore stoi error */ }
         }
         // Handle names without parentheses (e.g., "ClosePrice") if needed
         return {indicator_str, -1}; // Return full string and -1 if no period found/parsed
     }

    std::unique_ptr<indicators::IIndicator> Backtester::createIndicator(const std::string& name) {
         auto logger = core::logging::getLogger();
         logger->debug("Attempting to create indicator instance for: {}", name);

         auto parsed = parseIndicatorString(name);
         std::string base_name = parsed.first;
         int period = parsed.second;

         // Simple factory logic (expand later with more indicators)
         if (base_name == "SMA") {
             if (period <= 0) {
                 logger->error("Invalid period {} for SMA indicator '{}'", period, name);
                 return nullptr;
             }
             logger->debug("Creating SMAIndicator({})", period);
             return std::make_unique<indicators::SmaIndicator>(period);
         }
         // --- Add cases for other indicators ---
         // else if (base_name == "RSI") {
         //    if (period <= 0) { /* error */ return nullptr; }
         //    logger->debug("Creating RsiIndicator({})", period);
         //    return std::make_unique<indicators::RsiIndicator>(period); // Requires RsiIndicator implementation
         // }
         // else if (base_name == "MACD") { ... need multiple periods ... }

         logger->error("Unknown indicator name requested by strategy factory: {}", name);
         return nullptr; // Failed to create
    }

    bool Backtester::createAndCalculateIndicators() {
        auto logger = core::logging::getLogger();
        logger->info("Creating and calculating required indicators...");
        indicators_.clear();
        indicator_results_.clear();
    
        if (!strategy_) {
             logger->error("Cannot create indicators: Strategy not loaded.");
             return false;
        }
        if (primary_data_.empty()) {
             logger->error("Cannot calculate indicators: No primary data loaded.");
             return false;
        }
    
        const auto& required_names = strategy_->getRequiredIndicatorNames();
        if (required_names.empty()) {
             logger->info("No specific indicators required by strategy.");
             return true; // Nothing to do
        }
    
        logger->debug("Strategy requires indicators: {}", fmt::join(required_names, ", "));
    
        bool all_successful = true;
        for (const std::string& name : required_names) {
             logger->debug("Processing required indicator: {}", name);
             auto indicator = createIndicator(name); // Use helper factory method
             if (!indicator) {
                  logger->error("Failed to create indicator instance for '{}'. Backtest cannot proceed accurately.", name);
                  all_successful = false;
                  // Decide: continue without indicator, or stop backtest? Let's stop for now.
                  // To continue, use: continue;
                  return false; // Stop if any required indicator fails
             }
    
             logger->info("Calculating indicator: {}", indicator->getName()); // Use name from instance
             try {
                if (primary_data_.size() <= static_cast<size_t>(indicator->getLookback())) {
                     logger->error("Not enough primary data ({}) to calculate indicator '{}' which needs lookback {}.",
                                  primary_data_.size(), indicator->getName(), indicator->getLookback());
                     all_successful = false;
                     return false; // Stop if not enough data for calculation
                }
    
                indicator->calculate(primary_data_); // Calculate using loaded data
    
                // Store results - make a copy
                indicator_results_[indicator->getName()] = indicator->getResult();
                // Store the indicator instance itself (for lookback info etc.)
                indicators_[indicator->getName()] = std::move(indicator);
    
                 logger->info(" -> Calculated {} result points for {}.",
                              indicator_results_[indicators_[name]->getName()].size(), // Get name again safely
                              indicators_[name]->getName()); // Use name from stored indicator
    
             } catch (const std::exception& e) {
                  logger->error("Exception calculating indicator '{}': {}", name, e.what());
                  all_successful = false;
                   return false; // Stop if calculation fails
             }
        }
        return all_successful;
    }

    // --- Implement runEventLoop ---
    void Backtester::runEventLoop() {
        auto logger = core::logging::getLogger();

        if (primary_data_.empty() || !strategy_ || indicators_.size() != strategy_->getRequiredIndicatorNames().size()) {
            logger->error("Backtest prerequisites not met (data/strategy/indicators). Cannot run event loop.");
            return;
        }

        // --- Determine Maximum Lookback ---
        int max_lookback = 0;
        for (const auto& pair : indicators_) {
            if (pair.second) { // Check if indicator pointer is valid
                max_lookback = std::max(max_lookback, pair.second->getLookback());
            }
        }
        logger->info("Maximum indicator lookback period: {}", max_lookback);

        // --- Check if enough data exists ---
        if (primary_data_.size() <= static_cast<size_t>(max_lookback)) {
            logger->error("Not enough primary data ({}) to cover maximum lookback ({}). Cannot run loop.",
                        primary_data_.size(), max_lookback);
            return;
        }

        // --- Main Event Loop ---
        logger->info("Iterating through {} bars (starting after lookback)...", primary_data_.size() - max_lookback);
        // Start loop from the first index where ALL indicators have a valid value
        for (size_t i = static_cast<size_t>(max_lookback); i < primary_data_.size(); ++i) {

            const core::Candle& current_candle = primary_data_[i];

            // --- 1. Create Market Data Snapshot ---
            strategy_engine::MarketDataSnapshot snapshot;
            snapshot.current_time = current_candle.timestamp;
            snapshot.current_candle = &current_candle;

            // Populate indicator values for the *current* candle time 'i'
            bool indicators_ready = true; // Flag to check if all needed indicators have values
            for (const auto& pair : indicators_) {
                const std::string& name = pair.first;
                const auto& indicator = pair.second;
                if (!indicator) continue; // Should not happen if createIndicators worked

                // The result index corresponds to the input index minus the lookback
                int result_index = static_cast<int>(i) - indicator->getLookback();

                const auto& results = indicator_results_[name]; // Get results vector for this indicator

                if (result_index >= 0 && static_cast<size_t>(result_index) < results.size()) {
                    snapshot.indicator_values[name] = results[static_cast<size_t>(result_index)];
                    logger->trace("Snapshot Time: {}, Indicator: {}, Index: {}, Value: {}",
                                core::utils::timestampToString(snapshot.current_time),
                                name, result_index, snapshot.indicator_values[name]);
                } else {
                    // Indicator value not available yet for this candle (still in lookback period for *this* indicator)
                    // OR calculation somehow produced fewer results than expected.
                    logger->trace("Snapshot Time: {}, Indicator: {}, Value: N/A (Result Index {})",
                                core::utils::timestampToString(snapshot.current_time), name, result_index);
                    indicators_ready = false; // Mark snapshot as potentially incomplete for this step
                    // Strategy evaluation might handle missing indicators, or we could skip evaluation
                }
            }

            // Optional: Skip evaluation if not all indicators are ready?
            // if (!indicators_ready) {
            //      logger->trace("Skipping strategy evaluation at {} due to lack of indicator data.", core::utils::timestampToString(snapshot.current_time));
            //      continue;
            // }


            // --- 2. Evaluate Strategy ---
            core::SignalAction signal = strategy_->evaluate(snapshot);


            // --- 3. Execute Signal (Placeholder) ---
            if (signal != core::SignalAction::None) {
                logger->info("Time: {}, Signal Generated: {}", core::utils::timestampToString(snapshot.current_time), static_cast<int>(signal));
                executeSignal(snapshot.current_time, current_candle, signal);
            }


            // --- 4. Record Portfolio Value for this Timestamp ---
            // Create map of current prices (use close price for simplicity)
            std::map<std::string, double> current_prices;
            current_prices[primary_instrument_key_] = current_candle.close;
            portfolio_->recordTimestampValue(snapshot.current_time, current_prices);


            // --- Loop End ---
        } // End for loop through primary_data_

    } // End runEventLoop

    void Backtester::executeSignal(core::Timestamp timestamp, const core::Candle& current_candle, core::SignalAction signal) {
        auto logger = core::logging::getLogger();
        logger->debug("Executing Signal: Time={}, Signal={}, Candle Close={:.2f}",
                     core::utils::timestampToString(timestamp), static_cast<int>(signal), current_candle.close);
  
        // --- Simple Execution & Sizing Logic ---
        // TODO: Replace with more sophisticated logic later
        long long current_position = portfolio_->getPositionQuantity(primary_instrument_key_);
        long long quantity_to_trade = 0;
        double execution_price = current_candle.close; // Simple fill at close
        double commission = 0.01 * execution_price;     // Example: 0.01 currency unit commission per share
  
        // --- Basic Sizing: Trade fixed 10 shares for now ---
        long long trade_size = 10;
  
        // Determine quantity based on signal and current position
        switch(signal) {
            case core::SignalAction::EnterLong:
                if (current_position == 0) { // Only enter if flat
                     quantity_to_trade = trade_size;
                     logger->info("-> Attempting to BUY {} shares", quantity_to_trade);
                } else { logger->debug("Ignoring EnterLong signal, already have position {}.", current_position); }
                break;
            case core::SignalAction::ExitLong:
                 if (current_position > 0) { // Only exit if long
                     quantity_to_trade = current_position; // Exit entire position
                     logger->info("-> Attempting to SELL {} shares (Exit Long)", quantity_to_trade);
                } else { logger->debug("Ignoring ExitLong signal, not currently long."); }
                break;
            case core::SignalAction::EnterShort:
                 if (current_position == 0) { // Only enter if flat
                     quantity_to_trade = trade_size; // Sell this many to enter short
                     logger->info("-> Attempting to SELL {} shares (Enter Short)", quantity_to_trade);
                } else { logger->debug("Ignoring EnterShort signal, already have position {}.", current_position); }
                break;
            case core::SignalAction::ExitShort:
                  if (current_position < 0) { // Only exit if short
                     quantity_to_trade = -current_position; // Buy back entire position (positive quantity)
                     logger->info("-> Attempting to BUY {} shares (Exit Short)", quantity_to_trade);
                } else { logger->debug("Ignoring ExitShort signal, not currently short."); }
                break;
            case core::SignalAction::None:
            default:
                break; // No trade
        }
  
        // If a trade was determined, record it
        if (quantity_to_trade != 0) {
             // Pass the original signal to recordTrade to determine cost direction
             portfolio_->recordTrade(timestamp, primary_instrument_key_, signal,
                                     std::abs(quantity_to_trade), // Quantity is always positive in recordTrade
                                     execution_price, commission);
        }
    }

    void Backtester::calculateMetrics() {
        auto logger = core::logging::getLogger();
        logger->info("Calculating performance metrics...");
   
        const auto& equity_curve = portfolio_->getEquityCurve();
        if (equity_curve.size() < 2) {
            logger->warn("Not enough equity points ({}) to calculate metrics.", equity_curve.size());
            // Set default metrics? For now, just return.
            return;
        }
   
        BacktestMetrics metrics; // Create instance to store results
   
        // --- Total Return ---
        double final_equity = equity_curve.back().total_equity;
        metrics.total_return_pct = (final_equity - initial_capital_) / initial_capital_;

        metrics.total_pnl = final_equity - initial_capital_;
   
        // --- Max Drawdown ---
        double peak_equity = initial_capital_;
        double max_drawdown = 0.0; // Max drawdown observed so far (positive value)
   
        for (const auto& state : equity_curve) {
            peak_equity = std::max(peak_equity, state.total_equity);
            double current_drawdown = (peak_equity > 1e-9) ? (peak_equity - state.total_equity) / peak_equity : 0.0; // Avoid division by zero/small numbers
            max_drawdown = std::max(max_drawdown, current_drawdown);
        }
        metrics.max_drawdown_pct = max_drawdown;
   
        // --- Total Trades ---
        metrics.total_trades = portfolio_->getTotalTrades();

        logger->debug("Metrics Values: FinalEquity={:.4f}, InitialCapital={:.4f}, Calculated PnL={:.4f}, PctReturn={:.6f}, MaxDD={:.6f}, Trades={}",
            final_equity,
            initial_capital_,
            metrics.total_pnl, // Log the value stored in the struct
            metrics.total_return_pct,
            metrics.max_drawdown_pct,
            metrics.total_trades);
   
   
        // --- Log Metrics ---
        metrics.logMetrics(); // Use helper function to log
   
        // Store metrics? Make accessible via getter?
        // For now, just logging. Add member variable 'BacktestMetrics results_' if needed.
   
   }

    // Getter added for completeness, might need adjustment
    const Portfolio& Backtester::getPortfolio() const {
         if (!portfolio_) {
              throw std::runtime_error("Portfolio accessed before initialization in backtester.");
         }
         return *portfolio_;
    }


} // namespace backtester
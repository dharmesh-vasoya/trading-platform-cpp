#include "backtester.hpp"
#include "strategy_factory.hpp" // Need factory to create strategy
#include "sma_indicator.hpp"    // Need concrete indicators for createIndicator factory <<< USE SHORT PATH
#include "rsi_indicator.hpp"
#include "common_types.hpp" 
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
         } else if (base_name == "RSI") {
            if (period <= 0) {
                  logger->error("Invalid period {} for RSI indicator '{}'", period, name);
                  return nullptr;
            }
            logger->debug("Creating RsiIndicator({})", period);
            return std::make_unique<indicators::RsiIndicator>(period);
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
        if (!strategy_ || !portfolio_) {
             logger->error("Cannot execute signal: Strategy or Portfolio not initialized.");
             return;
        }
    
        logger->debug("Executing Signal: Time={}, Signal={}, Candle Close={:.2f}",
                     core::utils::timestampToString(timestamp), static_cast<int>(signal), current_candle.close);
    
        long long current_position = portfolio_->getPositionQuantity(primary_instrument_key_);
        double execution_price = current_candle.close; // Simple fill at close
        // TODO: Make commission configurable (e.g., strategy param or backtester setting)
        double commission_per_share = 0.01; // Example fixed commission per share
        long long quantity_to_trade = 0; // This will be calculated based on sizing
    
        // --- Get Sizing Parameters from Strategy ---
        auto sizing_method = strategy_->getSizingMethod();
        double sizing_value = strategy_->getSizingValue();
        bool sizing_is_percentage = strategy_->isSizingValuePercentage();
    
    
        // --- Calculate Trade Quantity based on Sizing Method ---
        if (signal == core::SignalAction::EnterLong || signal == core::SignalAction::EnterShort) {
             if (current_position != 0) {
                 logger->debug("Ignoring Entry signal [{}] because position is not flat ({}).", static_cast<int>(signal), current_position);
                 return; // Ignore entry if already in position
             }
    
             if (sizing_method == strategy_engine::SizingMethod::Quantity) {
                 quantity_to_trade = static_cast<long long>(sizing_value);
             } else if (sizing_method == strategy_engine::SizingMethod::CapitalBased) {
                  double capital_to_allocate = 0.0;
                  if (sizing_is_percentage) {
                       // Use Initial Capital for max allocation % (like Streak often does)
                       // Alternatively use current equity: portfolio_->getCurrentEquity(...) - requires prices map
                       capital_to_allocate = initial_capital_ * (sizing_value / 100.0);
                  } else {
                       capital_to_allocate = sizing_value; // Absolute amount
                  }
    
                  if (execution_price > 1e-9) { // Avoid division by zero/tiny price
                      quantity_to_trade = static_cast<long long>(std::floor(capital_to_allocate / execution_price));
                  } else {
                       logger->error("Cannot calculate quantity: Execution price is too low ({}).", execution_price);
                       quantity_to_trade = 0;
                  }
             } else {
                  logger->error("Unknown sizing method encountered in executeSignal.");
                   quantity_to_trade = 0;
             }
    
              if (quantity_to_trade <= 0) {
                   logger->warn("Calculated entry quantity is zero or negative ({}). Ignoring signal.", quantity_to_trade);
                   quantity_to_trade = 0; // Ensure it's 0 if calculation failed
              }
    
        } else if (signal == core::SignalAction::ExitLong) {
             if (current_position > 0) { // Only exit if long
                 quantity_to_trade = current_position; // Exit entire position
             } else { logger->debug("Ignoring ExitLong signal, not currently long."); return; }
        } else if (signal == core::SignalAction::ExitShort) {
             if (current_position < 0) { // Only exit if short
                 quantity_to_trade = -current_position; // Buy back entire position (positive quantity)
             } else { logger->debug("Ignoring ExitShort signal, not currently short."); return; }
        } else {
             return; // No action for SignalAction::None
        }
    
    
        // --- Record Trade if Quantity > 0 ---
        if (quantity_to_trade > 0) {
             // Calculate commission for this leg
             double commission = commission_per_share * quantity_to_trade;
    
             // Log attempt
             if (signal == core::SignalAction::EnterLong || signal == core::SignalAction::ExitShort) {
                  logger->info("-> Attempting to BUY {} shares", quantity_to_trade);
             } else { // EnterShort or ExitLong
                  logger->info("-> Attempting to SELL {} shares ({})", quantity_to_trade,
                               (signal == core::SignalAction::EnterShort ? "Enter Short" : "Exit Long"));
             }
    
             // Pass the original signal, positive quantity, price, commission
             portfolio_->recordTrade(timestamp, primary_instrument_key_, signal,
                                     quantity_to_trade,
                                     execution_price, commission);
        } else {
             logger->debug("No trade executed for signal [{}] (calculated quantity={}).", static_cast<int>(signal), quantity_to_trade);
        }
    }

    void Backtester::calculateMetrics() {
        auto logger = core::logging::getLogger();
        logger->info("Calculating performance metrics...");
   
        const auto& equity_curve = portfolio_->getEquityCurve();
        const auto& trade_log = portfolio_->getTradeLog(); // Get the completed trades
   
        if (equity_curve.size() < 2) {
            logger->warn("Not enough equity points ({}) to calculate metrics.", equity_curve.size());
            return;
        }
   
        BacktestMetrics metrics; // Create instance to store results
        metrics.total_executions = portfolio_->getTotalExecutions();
   
        // --- PnL and Return ---
        double final_equity = equity_curve.back().total_equity;
        // Ensure any open positions are marked-to-market for final equity?
        // getCurrentEquity already does this based on last candle price passed to recordTimestampValue.
        metrics.total_pnl = final_equity - initial_capital_;
        metrics.total_return_pct = (initial_capital_ > 1e-9) ? metrics.total_pnl / initial_capital_ : 0.0;
   
        // --- Max Drawdown ---
        double peak_equity = initial_capital_;
        double max_drawdown = 0.0;
        for (const auto& state : equity_curve) {
            peak_equity = std::max(peak_equity, state.total_equity);
            double current_drawdown = (peak_equity > 1e-9) ? (peak_equity - state.total_equity) / peak_equity : 0.0;
            max_drawdown = std::max(max_drawdown, current_drawdown);
        }
        metrics.max_drawdown_pct = max_drawdown;
   
        // --- Trade-Based Metrics ---
        metrics.round_trip_trades = static_cast<int>(trade_log.size()); // Count completed round trips
        int winning_trades = 0;
        double gross_profit = 0.0;
        double gross_loss = 0.0;
   
        for (const auto& trade : trade_log) {
             if (trade.pnl > 0) {
                 winning_trades++;
                 gross_profit += trade.pnl;
             } else if (trade.pnl < 0) {
                  gross_loss += trade.pnl; // Loss is negative
             }
             // Ignore trades with PnL == 0 for win rate etc.
        }
   
        int losing_trades = metrics.round_trip_trades - winning_trades; // Excludes zero PnL trades
   
        if (metrics.round_trip_trades > 0) {
            metrics.win_rate = static_cast<double>(winning_trades) / metrics.round_trip_trades;
        } else {
             metrics.win_rate = 0.0;
        }
   
        if (std::abs(gross_loss) > 1e-9) {
            metrics.profit_factor = gross_profit / std::abs(gross_loss);
        } else if (gross_profit > 1e-9) {
             metrics.profit_factor = std::numeric_limits<double>::infinity(); // Or some large number
        } else {
             metrics.profit_factor = 0.0; // Or 1.0 if no profit/loss? Define convention.
        }
   
   
         metrics.avg_win_pnl = (winning_trades > 0) ? gross_profit / winning_trades : 0.0;
         metrics.avg_loss_pnl = (losing_trades > 0) ? gross_loss / losing_trades : 0.0; // Will be negative
   
   
        // --- Sharpe Ratio (Simplified Example - Daily Data) ---
        // Requires calculating periodic returns from the equity curve
        if (equity_curve.size() > 1) {
             std::vector<double> daily_returns;
             daily_returns.reserve(equity_curve.size() - 1);
             for (size_t i = 1; i < equity_curve.size(); ++i) {
                  if (equity_curve[i-1].total_equity > 1e-9) { // Avoid division by zero
                       daily_returns.push_back((equity_curve[i].total_equity / equity_curve[i-1].total_equity) - 1.0);
                  } else {
                       daily_returns.push_back(0.0);
                  }
             }
   
             if (daily_returns.size() > 1) {
                  double sum_returns = std::accumulate(daily_returns.begin(), daily_returns.end(), 0.0);
                  double mean_return = sum_returns / daily_returns.size();
   
                  double sq_sum = std::inner_product(daily_returns.begin(), daily_returns.end(), daily_returns.begin(), 0.0);
                  double std_dev = std::sqrt(sq_sum / daily_returns.size() - mean_return * mean_return);
   
                  if (std_dev > 1e-9) {
                       // Assuming 0% risk-free rate and ~252 trading days per year
                       double risk_free_rate_daily = 0.0;
                       double annualization_factor = std::sqrt(252.0); // Adjust if data isn't daily
                       // metrics.sharpe_ratio = annualization_factor * (mean_return - risk_free_rate_daily) / std_dev;
                       // Let's skip Sharpe for now as it adds complexity
                  }
             }
        }
   
   
        // --- Log Metrics ---
        metrics.logMetrics();
   
        // Store metrics in Backtester? Add member variable BacktestMetrics results_;
   }

    // Getter added for completeness, might need adjustment
    const Portfolio& Backtester::getPortfolio() const {
         if (!portfolio_) {
              throw std::runtime_error("Portfolio accessed before initialization in backtester.");
         }
         return *portfolio_;
    }


} // namespace backtester
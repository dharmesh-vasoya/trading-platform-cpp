#include "rsi_indicator.hpp" // Use short path
#include "exceptions.hpp"    // Use short path
#include "logging.hpp"     // Use short path
#include "ta_libc.h"  // TA-Lib C API header
#include <vector>
#include <stdexcept>
#include "spdlog/fmt/bundled/core.h" // Direct path for fmt safety

namespace indicators {

RsiIndicator::RsiIndicator(int period) : period_(period), lookback_(0) {
    if (period_ <= 0) {
         throw std::invalid_argument("RSI period must be positive.");
    }

    // Determine the lookback period required by TA-Lib
    lookback_ = TA_RSI_Lookback(period_);
    if (lookback_ < 0) {
         throw std::runtime_error(fmt::format("TA_RSI_Lookback returned an unexpected value: {}", lookback_));
    }

    name_ = fmt::format("RSI({})", period_);
    results_.clear();
    core::logging::getLogger()->debug("RsiIndicator created: Name='{}', Period={}, Lookback={}", name_, period_, lookback_);
}

std::string RsiIndicator::getName() const {
    return name_;
}

int RsiIndicator::getLookback() const {
    return lookback_;
}

const core::TimeSeries<double>& RsiIndicator::getResult() const {
    return results_;
}

void RsiIndicator::calculate(const core::TimeSeries<core::Candle>& input) {
    auto logger = core::logging::getLogger();
    logger->trace("Calculating {}...", name_);
    results_.clear();

    // Check input size
    if (input.size() <= static_cast<size_t>(lookback_)) {
        logger->debug("Input size ({}) is less than or equal to lookback ({}) for {}. No results generated.",
                      input.size(), lookback_, name_);
        return;
    }

    // Prepare input array (closing prices)
    std::vector<double> close_prices;
    close_prices.reserve(input.size());
    for (const auto& candle : input) {
        close_prices.push_back(candle.close);
    }

    // Prepare output array
    int output_size = static_cast<int>(close_prices.size()) - lookback_;
     if (output_size <= 0) {
         logger->warn("Output size calculated as {} for {}. No results generated.", output_size, name_);
         return;
    }
    results_.resize(output_size);

    // TA-Lib output parameters
    int out_begin_idx = 0;
    int out_nb_element = 0;

    // Call the TA-Lib function for RSI
    TA_RetCode ret_code = TA_RSI(
        0,                                     // startIdx
        static_cast<int>(close_prices.size()) - 1, // endIdx
        close_prices.data(),                   // Pointer to input data
        period_,                               // optInTimePeriod
        &out_begin_idx,                        // outBegIdx
        &out_nb_element,                       // outNbElement
        results_.data()                        // outReal: Pointer to output buffer
    );

    // Check return code
    if (ret_code != TA_SUCCESS) {
        logger->error("TA-Lib TA_RSI calculation failed for {} with error code: {}", name_, static_cast<int>(ret_code));
        results_.clear();
        return;
    }

    // Verify output parameters
    if (out_begin_idx != lookback_) {
         logger->warn("TA_RSI out_begin_idx ({}) does not match calculated lookback ({}) for {}. Results might be misaligned.",
                      out_begin_idx, lookback_, name_);
    }
    if (out_nb_element != output_size) {
         logger->warn("TA_RSI out_nb_element ({}) does not match expected output size ({}) for {}. Resizing results vector.",
                      out_nb_element, output_size, name_);
         results_.resize(out_nb_element); // Adjust size if needed
    }

    logger->trace("Successfully calculated {} results for {}", results_.size(), name_);
}

} // namespace indicators
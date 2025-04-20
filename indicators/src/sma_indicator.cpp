#include "sma_indicator.hpp"
#include "exceptions.hpp"   // For potential exceptions
#include "logging.hpp"    // For logging errors
#include "ta_libc.h"            // Include TA-Lib C API header
#include <vector>
#include <stdexcept>                   // For std::runtime_error
#include <spdlog/spdlog.h>                 // For formatting error messages

namespace indicators {

SmaIndicator::SmaIndicator(int period) : period_(period), lookback_(0) {
    if (period_ <= 0) {
         throw std::invalid_argument("SMA period must be positive.");
    }

    // Determine the lookback period required by TA-Lib for this period
    lookback_ = TA_MA_Lookback(period_, TA_MAType_SMA); // Use TA_MAType_SMA for simple moving average
    if (lookback_ < 0) {
        // This shouldn't typically happen for valid periods with TA_MA_Lookback
         throw std::runtime_error(fmt::format("TA_MA_Lookback returned an unexpected value: {}", lookback_));
    }

    name_ = fmt::format("SMA({})", period_);
    results_.clear(); // Ensure results are initially empty
     core::logging::getLogger()->debug("SmaIndicator created: Name='{}', Period={}, Lookback={}", name_, period_, lookback_);
}

std::string SmaIndicator::getName() const {
    return name_;
}

int SmaIndicator::getLookback() const {
    return lookback_;
}

const core::TimeSeries<double>& SmaIndicator::getResult() const {
    return results_;
}

void SmaIndicator::calculate(const core::TimeSeries<core::Candle>& input) {
    auto logger = core::logging::getLogger();
    logger->trace("Calculating {}...", name_);
    results_.clear(); // Clear previous results

    // Check if input size is sufficient
    if (input.size() <= static_cast<size_t>(lookback_)) {
        logger->debug("Input size ({}) is less than or equal to lookback ({}) for {}. No results generated.",
                      input.size(), lookback_, name_);
        return; // Not enough data to calculate anything
    }

    // Prepare input array (closing prices) for TA-Lib
    std::vector<double> close_prices;
    close_prices.reserve(input.size());
    for (const auto& candle : input) {
        close_prices.push_back(candle.close);
    }

    // Prepare output array
    // TA-Lib output size = input size - lookback
    int output_size = static_cast<int>(close_prices.size()) - lookback_;
    if (output_size <= 0) {
         logger->warn("Output size calculated as {} for {}. No results generated.", output_size, name_);
         return; // Should technically be caught by the first size check, but good sanity check
    }
    results_.resize(output_size); // Resize internal results vector directly

    // TA-Lib output parameters
    int out_begin_idx = 0; // Index of the first valid output element relative to input
    int out_nb_element = 0; // Number of elements calculated

    // Call the TA-Lib function for Simple Moving Average (TA_MA)
    TA_RetCode ret_code = TA_MA(
        0,                                     // startIdx: Start from the beginning of the input array
        static_cast<int>(close_prices.size()) - 1, // endIdx: Process up to the last element
        close_prices.data(),                   // Pointer to input data (closing prices)
        period_,                               // optInTimePeriod: The period for SMA
        TA_MAType_SMA,                         // optInMAType: Specify Simple Moving Average
        &out_begin_idx,                        // outBegIdx: Index of the first output candle
        &out_nb_element,                       // outNbElement: Number of output elements calculated
        results_.data()                        // outReal: Pointer to the output buffer (our results_ vector)
    );

    // Check the return code
    if (ret_code != TA_SUCCESS) {
        logger->error("TA-Lib TA_MA calculation failed for {} with error code: {}", name_, static_cast<int>(ret_code));
        results_.clear(); // Clear potentially partial results on error
        // Consider throwing an exception:
        // throw core::IndicatorCalculationException(fmt::format("TA_MA failed for {} with code {}", name_, ret_code));
        return;
    }

    // Verify output parameters (sanity checks)
    if (out_begin_idx != lookback_) {
         logger->warn("TA_MA out_begin_idx ({}) does not match calculated lookback ({}) for {}. Results might be misaligned.",
                      out_begin_idx, lookback_, name_);
         // Adjust internal lookback? Or handle potential misalignment? For SMA, they should match.
    }
     if (out_nb_element != output_size) {
         logger->warn("TA_MA out_nb_element ({}) does not match expected output size ({}) for {}. Resizing results vector.",
                      out_nb_element, output_size, name_);
         // Resize results_ if TA-Lib returned fewer elements than expected
         results_.resize(out_nb_element);
    }

    logger->trace("Successfully calculated {} results for {}", results_.size(), name_);
}

} // namespace indicators
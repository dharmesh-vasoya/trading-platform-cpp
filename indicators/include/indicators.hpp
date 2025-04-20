#pragma once

#include "core/include/datatypes.hpp" // Needs Candle, TimeSeries
#include <string>
#include <vector>
#include <any> // Or use specific types for results

namespace indicators {

// Forward declare if needed, or include directly
// class core::TimeSeries<core::Candle>;
// class core::TimeSeries<double>; // Example result type

class IIndicator {
public:
    virtual ~IIndicator() = default; // Virtual destructor is important for interfaces!

    // Get the name of the indicator (e.g., "SMA", "RSI")
    virtual std::string getName() const = 0;

    // Get the lookback period required by the indicator calculation
    // This determines how many initial input data points are consumed
    // before the first valid output can be generated.
    virtual int getLookback() const = 0;

    // Calculate the indicator based on input candle data
    // It should store the result internally.
    virtual void calculate(const core::TimeSeries<core::Candle>& input) = 0;

    // Get the calculated results.
    // Returns a vector of doubles. The size might be smaller than input
    // due to the lookback period. Caller needs to align based on lookback.
    // Consider returning a struct with alignment info if needed.
    virtual const core::TimeSeries<double>& getResult() const = 0;

    // Optional: Add methods for specific results if indicator returns multiple lines (e.g., MACD)
    // virtual const core::TimeSeries<double>& getSignalLine() const = 0; // Example for MACD
};

} // namespace indicators#pragma once

#include "core/include/datatypes.hpp" // Needs Candle, TimeSeries
#include <string>
#include <vector>
#include <any> // Or use specific types for results

namespace indicators {

// Forward declare if needed, or include directly
// class core::TimeSeries<core::Candle>;
// class core::TimeSeries<double>; // Example result type

class IIndicator {
public:
    virtual ~IIndicator() = default; // Virtual destructor is important for interfaces!

    // Get the name of the indicator (e.g., "SMA", "RSI")
    virtual std::string getName() const = 0;

    // Get the lookback period required by the indicator calculation
    // This determines how many initial input data points are consumed
    // before the first valid output can be generated.
    virtual int getLookback() const = 0;

    // Calculate the indicator based on input candle data
    // It should store the result internally.
    virtual void calculate(const core::TimeSeries<core::Candle>& input) = 0;

    // Get the calculated results.
    // Returns a vector of doubles. The size might be smaller than input
    // due to the lookback period. Caller needs to align based on lookback.
    // Consider returning a struct with alignment info if needed.
    virtual const core::TimeSeries<double>& getResult() const = 0;

    // Optional: Add methods for specific results if indicator returns multiple lines (e.g., MACD)
    // virtual const core::TimeSeries<double>& getSignalLine() const = 0; // Example for MACD
};

} // namespace indicators
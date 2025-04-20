#pragma once

#include "indicators.hpp" // Base interface
#include <vector>
#include <string>
#include <stdexcept> // For potential errors

namespace indicators {

class SmaIndicator : public IIndicator {
public:
    // Constructor: Requires the period for the SMA
    explicit SmaIndicator(int period);

    // Override methods from IIndicator
    virtual ~SmaIndicator() override = default; // Use override keyword

    std::string getName() const override;
    int getLookback() const override;
    void calculate(const core::TimeSeries<core::Candle>& input) override;
    const core::TimeSeries<double>& getResult() const override;

private:
    const int period_;          // SMA period (e.g., 50, 200)
    int lookback_;              // Calculated TA-Lib lookback
    std::string name_;          // Indicator name (e.g., "SMA(50)")
    core::TimeSeries<double> results_; // Stores the calculated SMA values
};

} // namespace indicators
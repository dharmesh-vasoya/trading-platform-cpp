#pragma once

#include "indicators.hpp" // Use short path
#include <vector>
#include <string>
#include <stdexcept>

namespace indicators {

class RsiIndicator : public IIndicator {
public:
    // Constructor: Requires the period for the RSI
    explicit RsiIndicator(int period);

    // Override methods from IIndicator
    virtual ~RsiIndicator() override = default;

    std::string getName() const override;
    int getLookback() const override;
    void calculate(const core::TimeSeries<core::Candle>& input) override;
    const core::TimeSeries<double>& getResult() const override;

private:
    const int period_;
    int lookback_;
    std::string name_;
    core::TimeSeries<double> results_;
};

} // namespace indicators
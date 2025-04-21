#pragma once

namespace strategy_engine {

    // Enum to specify which candle price field to use
    enum class PriceField {
        Open,
        High,
        Low,
        Close
    };

    // Enum for comparison types
    enum class ComparisonOp {
        GT,  // Greater Than (>)
        LT,  // Less Than (<)
        GTE, // Greater Than or Equal To (>=)
        LTE, // Less Than or Equal To (<=)
        EQ   // Equal To (==)
    };

} // namespace strategy_engine
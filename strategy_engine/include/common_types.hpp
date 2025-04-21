#pragma once
#include "datatypes.hpp"     // Use short path (Provides core types)
#include "common_types.hpp"  // <<<--- ADD THIS INCLUDE (Provides SizingMethod enum)

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
    
    enum class SizingMethod {
        Quantity,      // Fixed number of shares/lots
        CapitalBased   // Allocate max capital (absolute or % of initial)
        // Add RiskBased later if needed
    };
    

} // namespace strategy_engine
# strategy_engine/CMakeLists.txt

add_library(strategy_engine STATIC
    src/price_condition.cpp
    src/indicator_condition.cpp 
    src/and_condition.cpp      
    src/or_condition.cpp
    src/rule.cpp
    src/strategy.cpp
    src/strategy_factory.cpp
    src/price_indicator_condition.cpp
    src/indicator_cross_condition.cpp
    # Add other .cpp files here later
)


# Specify include directories needed BY THIS TARGET'S sources
#target_include_directories(strategy_engine PRIVATE # PRIVATE is sufficient here
#    ${spdlog_SOURCE_DIR}/include # Add path to spdlog/fmt headers <<<--- ADD THIS
#)

# Specify include directories needed by CONSUMERS of this target
target_include_directories(strategy_engine PUBLIC include) # For interfaces.hpp etc.


# Specify dependencies needed by this module's code and/or headers
target_link_libraries(strategy_engine
     PUBLIC core
     PRIVATE spdlog::spdlog
     PRIVATE nlohmann_json::nlohmann_json
)

 # Ensure C++20 features are available
 target_compile_features(strategy_engine PRIVATE cxx_std_20)

message(STATUS "Configuring strategy_engine module (PriceCondition, IndicatorCondition)...") # Updated message

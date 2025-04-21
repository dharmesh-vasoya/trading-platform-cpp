# backtester/CMakeLists.txt

add_library(backtester STATIC
    src/portfolio.cpp   # Add portfolio source
    src/backtester.cpp  # Add backtester source
)

# Public include dir
target_include_directories(backtester PUBLIC include)

# Private include dirs needed by sources
target_include_directories(backtester PRIVATE
     # Add dependency include paths explicitly if needed
     ${spdlog_SOURCE_DIR}/include   # For logging/fmt used in .cpp
     # ta-lib headers should come via 'indicators' link below
     # json headers should come via 'strategy_engine' link below
)

# Link against all required modules and potentially base libraries
target_link_libraries(backtester PUBLIC # Public needed if CLI uses Backtester directly
    core              # For datatypes, logging, utils
    data              # For DatabaseManager
    indicators        # For IIndicator, SmaIndicator (pulls in ta_libc)
    strategy_engine   # For IStrategy, StrategyFactory etc (pulls in json)
    spdlog::spdlog    # Explicit link for logging in this module's cpp
    # nlohmann_json/cpr likely NOT needed directly here if only using Factory/DBMgr
)

 # Ensure C++20 features
 target_compile_features(backtester PRIVATE cxx_std_20)

message(STATUS "Configuring backtester module...")
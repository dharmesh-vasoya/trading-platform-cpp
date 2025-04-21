# backtester/CMakeLists.txt
add_library(backtester INTERFACE) 

# Add source files when they exist
# target_sources(backtester PRIVATE
#    src/backtest_runner.cpp
#    src/portfolio.cpp
# )

target_include_directories(backtester INTERFACE include)       # Use INTERFACE
target_compile_features(backtester INTERFACE cxx_std_20)     # Use INTERFACE
target_link_libraries(backtester INTERFACE core data indicators strategy_engine) # Use INTERFACE

message(STATUS "Configuring backtester module...")
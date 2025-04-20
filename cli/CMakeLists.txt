add_executable(trading_cli src/main.cpp) # Main executable

target_compile_features(trading_cli PRIVATE cxx_std_20)

# Link against modules it uses
target_link_libraries(trading_cli PRIVATE core data indicators strategy_engine backtester)
# Add external libs needed by cli (e.g., argument parser)

message(STATUS "Configuring cli module...")
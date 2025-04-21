# cli/CMakeLists.txt

add_executable(trading_cli src/main.cpp) # Main executable

target_compile_features(trading_cli PRIVATE cxx_std_20)

# Link against modules it uses
target_link_libraries(trading_cli PRIVATE
    core
    data
    indicators
    strategy_engine
    backtester
    ${CMAKE_BINARY_DIR}/libta_libc.a # Use the full path
    CLI11::CLI11
    # Other dependencies linked implicitly via PUBLIC should be okay
)

message(STATUS "Configuring cli module...")
# indicators/CMakeLists.txt

# Define this module's library target FIRST
# Contains C++ indicator wrappers
add_library(indicators STATIC
    src/sma_indicator.cpp
    # Add src/rsi_indicator.cpp etc. here later
)

# --- TA-Lib dependency target 'ta_libc' is now defined in the root CMakeLists.txt ---
# We just need to link against it below.

# --- Configure the 'indicators' library target ---

# Explicitly add spdlog's include path (contains fmt) because propagation sometimes fails
# Assumes FetchContent_MakeAvailable(spdlog) was called in root CMakeLists.txt
# ${spdlog_SOURCE_DIR} should be available from parent scope
target_include_directories(indicators PRIVATE
    ${spdlog_SOURCE_DIR}/include  # For fmt headers
    ${ta-lib_SOURCE_DIR}/include   # <-- ADD THIS LINE for ta_libc.h
)

# Add this module's public include directory
target_include_directories(indicators PUBLIC include)

# Ensure C++20 features are available for compiling indicator wrappers
target_compile_features(indicators PRIVATE cxx_std_20) # PRIVATE is usually fine here

# Link indicators library against 'core' and the globally defined 'ta_libc'
# PUBLIC ensures that targets linking against 'indicators' also link core and ta_libc
target_link_libraries(indicators PUBLIC core ${CMAKE_BINARY_DIR}/libta_libc.a)


message(STATUS "Configuring indicators module (using global TA-Lib target)...")
add_library(core STATIC src/logging.cpp src/utils.cpp) # Add more .cpp files as needed

target_include_directories(core PUBLIC include)

# Ensure C++20 features are available for this target
target_compile_features(core PRIVATE cxx_std_20) # PRIVATE if only needed internally

# --- Dependencies for core ---
FetchContent_MakeAvailable(spdlog)

# Link dependencies
# PUBLIC because consumers of 'core' might need spdlog headers via core's headers
target_link_libraries(core PUBLIC spdlog::spdlog)

message(STATUS "Configuring core module (STATIC library)...")
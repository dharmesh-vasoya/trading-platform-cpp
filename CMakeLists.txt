# Root CMakeLists.txt

# Minimum CMake version required
cmake_minimum_required(VERSION 3.20 FATAL_ERROR) # Use stable CMake 3.x

# Project definition
project(TradingPlatform VERSION 0.1.0 LANGUAGES CXX C) # <-- Added C here

# --- Dependency Management (FetchContent) ---
include(FetchContent)

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git # Or SSH URL
  GIT_TAG        v1.14.1
)

# nlohmann_json (Header-only JSON library)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git # Or SSH URL
  GIT_TAG        v3.11.3 # Use a specific release tag
)

# CPR (C++ Requests library)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git # Or SSH URL
  GIT_TAG        1.10.5 # Use a specific release tag
)

# TA-Lib C Library Source
FetchContent_Declare(
  ta-lib
  # Use the correct GitHub archive URL and hash for v0.4.0
  URL      https://github.com/TA-Lib/ta-lib/archive/v0.4.0.tar.gz
  URL_HASH SHA256=25b6ea9a1a89034e029e0f32490490de1827f7247c907be001325f8a0848e2ff
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git # Or SSH URL
  GIT_TAG        v2.3.2 # Use a specific release tag
)

# --- Build Shared Libs OFF ---
# Ensure all dependencies build as static libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries OFF")

# --- TA-Lib Target Definition (Moved here) ---
# Make TA-Lib source available
FetchContent_MakeAvailable(ta-lib)

# GLOB for TA-Lib source files
file(GLOB TA_FUNC_SOURCES    "${ta-lib_SOURCE_DIR}/src/ta_func/*.c")
file(GLOB TA_COMMON_SOURCES  "${ta-lib_SOURCE_DIR}/src/ta_common/*.c")

# Define the ta_libc static library target globally
add_library(ta_libc STATIC
    ${TA_FUNC_SOURCES}
    ${TA_COMMON_SOURCES}
)

# Configure the ta_libc target globally
target_include_directories(ta_libc
    PUBLIC  "${ta-lib_SOURCE_DIR}/include"      # Public API header (ta_libc.h)
    PRIVATE "${ta-lib_SOURCE_DIR}/src/ta_common" # Internal headers (ta_defs.h etc.)
)
target_compile_definitions(ta_libc PRIVATE TA_SINGLE_THREAD)
target_compile_options(ta_libc PRIVATE -w) # Suppress C warnings
# --- End TA-Lib Target Definition ---


# --- Standard Settings ---
# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Build Type ---
# Set a default build type if none was specified via -D CMAKE_BUILD_TYPE=...
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)." FORCE)
endif()

# --- Testing (CTest) ---
enable_testing() # Enable testing support

# --- Make Other Dependencies Available Before Subdirectories ---
# Ensure these are processed before subdirectories that might need them
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(nlohmann_json)
FetchContent_MakeAvailable(cpr)
FetchContent_MakeAvailable(CLI11)
# ta-lib is already handled above

# --- Subdirectories (Modules) ---
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(indicators) # Now just configures the 'indicators' target using global 'ta_libc'
add_subdirectory(strategy_engine)
add_subdirectory(backtester)
add_subdirectory(cli)
# add_subdirectory(tests) # Uncomment when adding tests

# --- Final Messages (Optional) ---
message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "CMake Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
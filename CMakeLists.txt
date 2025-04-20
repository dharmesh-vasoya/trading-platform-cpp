# Minimum CMake version required
cmake_minimum_required(VERSION 3.20 FATAL_ERROR) # Need a recent version for FetchContent and C++20 features

# Project definition
project(TradingPlatform VERSION 0.1.0 LANGUAGES CXX)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries OFF")

# --- Standard Settings ---
# Enforce C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard features

# --- Build Type ---
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)." FORCE)
endif()

# --- Dependency Management (FetchContent) ---
include(FetchContent)

# spdlog (already declared)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git # Or SSH URL
  GIT_TAG        v1.14.1
)
# nlohmann_json (Header-only JSON library - ADD THIS)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3 # Use a specific release tag
)

# CPR (C++ Requests library - ADD THIS)
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG        1.10.5 # Use a specific release tag
  # CPR requires some dependencies, FetchContent might handle them if CPR's CMake is setup for it
  # Or we might need to declare them too (e.g., libcurl) - let's try without first
)

# TA-Lib C Library Source (ADD THIS)
FetchContent_Declare(
  ta-lib
  URL      https://github.com/TA-Lib/ta-lib/archive/refs/tags/ta-lib-0.4.0.tar.gz # Official 0.4.0 source
  URL_HASH SHA256=79a9d0567fdf047177050e2f737567a719467f1c0348677e7d550e83685303c2 # Hash for 0.4.0 tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Note: Actual fetching (FetchContent_MakeAvailable) is usually done
# closer to where the dependency is needed (e.g., in core/CMakeLists.txt)
# or configured globally here if shared by many modules.

# --- Testing (CTest) ---
enable_testing()

# --- Subdirectories (Modules) ---
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(indicators)
add_subdirectory(strategy_engine)
add_subdirectory(backtester)
add_subdirectory(cli)
# add_subdirectory(web_api) # Uncomment if building API
# add_subdirectory(tests) # Add tests subdirectory last

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "CMake Generator: ${CMAKE_GENERATOR}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
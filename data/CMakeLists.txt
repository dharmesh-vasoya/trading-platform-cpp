# data/CMakeLists.txt

# --- Find Dependencies ---
find_package(SQLite3 REQUIRED)
FetchContent_MakeAvailable(nlohmann_json) # Make header-only lib available
FetchContent_MakeAvailable(cpr)           # Make CPR library available

# --- Data Library Target ---
# Add new source files for the API client here later
add_library(data STATIC
    src/database_manager.cpp
    src/upstox_api_client.cpp # Add new file
)

target_include_directories(data PUBLIC
    include # This module's own include dir
    # Add include dir provided by nlohmann_json (usually automatic via target_link_libraries)
)

target_compile_features(data PRIVATE cxx_std_20)

# Link 'data' against core, SQLite, nlohmann_json (INTERFACE), and cpr
target_link_libraries(data PUBLIC
     core
     SQLite::SQLite3
     nlohmann_json::nlohmann_json # Link INTERFACE target for header-only
     cpr::cpr                     # Link CPR library
)

message(STATUS "Configuring data module (using SQLite3, CPR, JSON)...")
#pragma once

#include <string>
#include <vector>
#include <memory>

// Remove DuckDB includes/forwards
// #include "duckdb.h"

// Add SQLite include
#include <sqlite3.h> // Standard C header

#include "datatypes.hpp" // Keep core types

namespace data {

class DatabaseManager {
public:
    explicit DatabaseManager(const std::string& db_path);
    ~DatabaseManager();

    // Delete copy constructor and assignment operator
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;
    // Default move constructor and assignment operator (optional)
    DatabaseManager(DatabaseManager&&) = default;
    DatabaseManager& operator=(DatabaseManager&&) = default;

    // Connect/Disconnect use SQLite API now
    bool connect();
    void disconnect();
    bool isConnected() const;

    // Schema init uses SQLite API now
    bool initializeSchema();

    // executeSQL uses SQLite API now
    bool executeSQL(const std::string& sql);

    bool saveCandles(const core::TimeSeries<core::Candle>& candles,
        const std::string& instrument_key,
        const std::string& interval);

    // queryCandles signature remains, implementation changes
    core::TimeSeries<core::Candle> queryCandles(
        const std::string& instrument_key,
        const std::string& interval,
        core::Timestamp start_time,
        core::Timestamp end_time); // Implementation later

    // Remove attachSQLite method
    // bool attachSQLite(const std::string& sqlite_path, const std::string& attach_name = "sqlite_db");

private:
    std::string database_path_;
    // Replace DuckDB handles with SQLite handle
    sqlite3* db_ = nullptr; // SQLite database connection handle
    bool connected_ = false;

    // Maybe add helper for SQLite errors later if needed
};

} // namespace data
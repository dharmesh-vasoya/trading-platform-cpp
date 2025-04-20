#include "database_manager.hpp"
#include "logging.hpp"
#include "exceptions.hpp"
#include "utils.hpp" // For timestampToString/stringToTimestamp
#include <vector>
#include <stdexcept> // For potential runtime_error on bad cast etc.
// Make sure sqlite3.h is included via database_manager.hpp
#include <iostream>
#include <chrono> // For time point conversions

namespace data
{

    DatabaseManager::DatabaseManager(const std::string &db_path)
        : database_path_(db_path), db_(nullptr), connected_(false) // Initialize db_ to nullptr
    {
        core::logging::getLogger()->debug("DatabaseManager (SQLite) created for path: {}", db_path);
    }

    DatabaseManager::~DatabaseManager()
    {
        core::logging::getLogger()->debug("DatabaseManager (SQLite) destructor called.");
        disconnect(); // Ensure disconnection
    }

    bool DatabaseManager::connect()
    {
        if (connected_)
        {
            core::logging::getLogger()->warn("Already connected to SQLite database {}.", database_path_);
            return true;
        }

        core::logging::getLogger()->info("Connecting to SQLite database: {}", database_path_);

        // SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE: Open for reading/writing, create if not exists
        // SQLITE_OPEN_NOMUTEX: Use thread-safe mode if your app might be multi-threaded later
        int rc = sqlite3_open_v2(database_path_.c_str(), &db_,
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                                 nullptr);

        if (rc != SQLITE_OK)
        {
            core::logging::getLogger()->error("Cannot open SQLite database '{}': {}", database_path_, sqlite3_errmsg(db_));
            sqlite3_close(db_); // Close handle even if open failed (as per docs)
            db_ = nullptr;
            return false;
        }

        connected_ = true;
        core::logging::getLogger()->info("Successfully connected to SQLite database: {}", database_path_);
        // Optional: Set busy timeout, WAL mode, etc.
        // sqlite3_busy_timeout(db_, 5000); // Wait 5 seconds if busy
        // executeSQL("PRAGMA journal_mode=WAL;"); // Enable Write-Ahead Logging for better concurrency
        return true;
    }

    void DatabaseManager::disconnect()
    {
        if (connected_)
        {
            core::logging::getLogger()->info("Disconnecting from SQLite database: {}", database_path_);
            int rc = sqlite3_close(db_); // Use sqlite3_close_v2 for more robust error handling if needed
            if (rc != SQLITE_OK)
            {
                // This usually happens if prepared statements are not finalized
                core::logging::getLogger()->error("Error disconnecting from SQLite database: {}", sqlite3_errmsg(db_));
                // You might still want to nullify the pointer
            }
            db_ = nullptr;
            connected_ = false;
        }
        else
        {
            core::logging::getLogger()->debug("Already disconnected (SQLite).");
        }
    }

    bool DatabaseManager::isConnected() const
    {
        // A more robust check might involve a simple PRAGMA query, but this is usually sufficient
        return connected_ && (db_ != nullptr);
    }

    bool DatabaseManager::executeSQL(const std::string &sql)
    {
        if (!isConnected())
        { // Use isConnected() helper
            core::logging::getLogger()->error("Cannot execute SQL: Not connected to database.");
            return false;
        }

        core::logging::getLogger()->debug("Executing SQL (SQLite): {}", sql);

        char *error_msg = nullptr;
        // sqlite3_exec is simpler for commands without results or where results aren't needed row-by-row
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_msg);

        if (rc != SQLITE_OK)
        {
            core::logging::getLogger()->error("SQL error: {}", error_msg);
            sqlite3_free(error_msg); // Must free error message memory
            return false;
        }

        core::logging::getLogger()->trace("SQL executed successfully (SQLite): {}", sql);
        return true;
    }

    bool DatabaseManager::initializeSchema()
    {
        if (!isConnected())
        {
            core::logging::getLogger()->error("Cannot initialize schema: Not connected to database.");
            return false;
        }
        core::logging::getLogger()->info("Initializing SQLite database schema if needed...");

        // Use the same SQL strings (SQLite supports CREATE TABLE IF NOT EXISTS)
        // Note: SQLite types are flexible (VARCHAR -> TEXT, DOUBLE -> REAL, BIGINT -> INTEGER)
        const std::string create_instruments_sql = R"(
        CREATE TABLE IF NOT EXISTS instruments (
            instrument_key TEXT PRIMARY KEY,
            exchange TEXT NOT NULL,
            segment TEXT NOT NULL,
            symbol TEXT NOT NULL,
            name TEXT,
            expiry_date TEXT, -- SQLite uses TEXT for dates
            strike_price REAL, -- SQLite uses REAL for floats
            option_type TEXT,
            lot_size INTEGER,
            tick_size REAL
        );
    )";

        const std::string create_candles_sql = R"(
        CREATE TABLE IF NOT EXISTS historical_candles (
            instrument_key TEXT,
            interval TEXT,
            timestamp TEXT, -- Store as ISO8601 TEXT or INTEGER Unix time
            open REAL,
            high REAL,
            low REAL,
            close REAL,
            volume INTEGER, -- SQLite uses INTEGER for various sizes
            open_interest INTEGER,
            PRIMARY KEY (instrument_key, interval, timestamp)
        );
    )";
        // SQLite benefits greatly from indexes, especially on timestamp
        const std::string create_candles_index_sql = R"(
        CREATE INDEX IF NOT EXISTS idx_candles_timestamp
        ON historical_candles (instrument_key, interval, timestamp);
     )";

        const std::string create_constituents_sql = R"(
        CREATE TABLE IF NOT EXISTS index_constituents (
            index_key TEXT,
            constituent_key TEXT,
            as_of_date TEXT, -- Store as TEXT YYYY-MM-DD
            PRIMARY KEY (index_key, constituent_key, as_of_date)
        );
    )";

        // Execute each statement
        bool success = true;
        success &= executeSQL(create_instruments_sql);
        success &= executeSQL(create_candles_sql);
        success &= executeSQL(create_candles_index_sql); // Add index creation
        success &= executeSQL(create_constituents_sql);

        if (success)
        {
            core::logging::getLogger()->info("SQLite database schema initialization check complete.");
        }
        else
        {
            core::logging::getLogger()->error("SQLite database schema initialization failed for one or more statements.");
        }
        return success;
    }

    // Remove the attachSQLite implementation entirely

    // queryCandles implementation needs complete rewrite using sqlite3_prepare_v2, etc.
    // Placeholder for now:
    // Replace the existing queryCandles function with this one:
    core::TimeSeries<core::Candle> DatabaseManager::queryCandles(
        const std::string &instrument_key,
        const std::string &interval,
        core::Timestamp start_time,
        core::Timestamp end_time)
    {
        core::TimeSeries<core::Candle> candles; // Vector to store results
        if (!isConnected())
        {
            core::logging::getLogger()->error("Cannot query candles: Not connected to database.");
            return candles; // Return empty vector
        }

        // Convert C++ Timestamps (std::chrono::system_clock, typically UTC based)
        // to Unix epoch seconds (integer) for binding
        auto start_epoch = std::chrono::duration_cast<std::chrono::seconds>(start_time.time_since_epoch()).count();
        auto end_epoch = std::chrono::duration_cast<std::chrono::seconds>(end_time.time_since_epoch()).count();

        core::logging::getLogger()->debug("Querying candles for {} ({}) between epoch {} ({}) and epoch {} ({})",
                                          instrument_key, interval, start_epoch, core::utils::timestampToString(start_time),
                                          end_epoch, core::utils::timestampToString(end_time));

        // Prepare the SQL statement
        // Compare timestamps by converting them to Unix epoch seconds within SQL using strftime('%s', ...)
        // const char* sql = R"(
        //     SELECT timestamp, open, high, low, close, volume -- No open_interest
        //     FROM historical_candles
        //     WHERE instrument_key = ?                     -- Placeholder 1
        //       AND interval = ?                           -- Placeholder 2
        //       AND strftime('%s', timestamp) >= ?         -- Placeholder 3 (compare epoch seconds)
        //       AND strftime('%s', timestamp) <= ?         -- Placeholder 4 (compare epoch seconds)
        //     ORDER BY timestamp ASC;
        // )";
        const char *sql = R"(
        SELECT timestamp, open, high, low, close, volume
        FROM historical_candles
        WHERE instrument_key = ?                     -- Placeholder 1
          AND interval = ?                           -- Placeholder 2
        ORDER BY timestamp ASC LIMIT 5; -- Limit to 5 rows, remove date filter
    )";

        sqlite3_stmt *stmt = nullptr;                              // Prepared statement handle
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr); // Prepare statement

        if (rc != SQLITE_OK)
        {
            core::logging::getLogger()->error("Failed to prepare SQL statement [{}]: {}", rc, sqlite3_errmsg(db_));
            sqlite3_finalize(stmt); // Finalize even if prepare failed
            return candles;         // Return empty
        }

        // Bind parameters
        // Index is 1-based
        sqlite3_bind_text(stmt, 1, instrument_key.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, interval.c_str(), -1, SQLITE_STATIC);
        // Bind start/end times as INTEGER (Unix epoch seconds)
        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(start_epoch));
        sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(end_epoch));

        // Execute the statement step-by-step and fetch rows
        core::logging::getLogger()->trace("Executing prepared statement for candle query...");
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
        {
            // A row of data is available
            try
            {
                core::Candle candle;

                // Retrieve data by column index (0-based)

                // Column 0: timestamp (Read as TEXT, parse using utils)
                const unsigned char *ts_text = sqlite3_column_text(stmt, 0);
                if (ts_text)
                {
                    core::logging::getLogger()->trace("Raw timestamp string from DB: {}", reinterpret_cast<const char *>(ts_text));
                    // Assuming stringToTimestamp can handle the "+05:30" offset correctly
                    // If not, stringToTimestamp might need adjustment.
                    candle.timestamp = core::utils::stringToTimestamp(reinterpret_cast<const char *>(ts_text));
                }
                else
                {
                    core::logging::getLogger()->warn("NULL timestamp found in query result, skipping row.");
                    continue; // Skip this row if timestamp is essential
                }

                // Read other columns as before (indexes shifted by 1 compared to original SELECT with OI)
                candle.open = sqlite3_column_double(stmt, 1);  // Index 1
                candle.high = sqlite3_column_double(stmt, 2);  // Index 2
                candle.low = sqlite3_column_double(stmt, 3);   // Index 3
                candle.close = sqlite3_column_double(stmt, 4); // Index 4
                candle.volume = sqlite3_column_int64(stmt, 5); // Index 5
                candle.open_interest = std::nullopt;           // No OI column selected

                candles.push_back(candle);
            }
            catch (const std::exception &e)
            {
                core::logging::getLogger()->error("Error processing row data: {}", e.what());
                // Decide whether to skip row or stop processing
            }
        }

        if (rc != SQLITE_DONE)
        {
            // sqlite3_step finished with an error other than completing successfully
            core::logging::getLogger()->error("Error stepping through query results [{}]: {}", rc, sqlite3_errmsg(db_));
        }
        else
        {
            core::logging::getLogger()->debug("Finished processing query results. Found {} candles.", candles.size());
        }

        // Finalize the statement to release resources
        sqlite3_finalize(stmt);

        return candles;
    }

    bool DatabaseManager::saveCandles(const core::TimeSeries<core::Candle> &candles,
                                      const std::string &instrument_key,
                                      const std::string &interval)
    {
        if (!isConnected())
        {
            core::logging::getLogger()->error("Cannot save candles: Not connected to database.");
            return false;
        }
        if (candles.empty())
        {
            core::logging::getLogger()->debug("No candles provided to save for {} ({}).", instrument_key, interval);
            return true; // Nothing to do, report success
        }

        core::logging::getLogger()->debug("Attempting to save/ignore {} candles for {} ({})", candles.size(), instrument_key, interval);

        // Prepare SQL statement for insertion, ignoring duplicates based on PRIMARY KEY
        // (instrument_key, interval, timestamp)
        const char *sql = R"(
INSERT OR IGNORE INTO historical_candles
(instrument_key, interval, timestamp, open, high, low, close, volume)
VALUES (?, ?, ?, ?, ?, ?, ?, ?);
)";

        sqlite3_stmt *stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr); // Prepare statement
        if (rc != SQLITE_OK)
        {
            core::logging::getLogger()->error("Failed to prepare INSERT statement [{}]: {}", rc, sqlite3_errmsg(db_));
            sqlite3_finalize(stmt); // Finalize even if prepare failed (safe if stmt is null)
            return false;
        }

        // Begin transaction for efficiency
        if (!executeSQL("BEGIN TRANSACTION;"))
        {
            core::logging::getLogger()->error("Failed to begin transaction for saving candles.");
            sqlite3_finalize(stmt);
            return false;
        }

        bool success = true;
        int saved_count = 0;
        for (const auto &candle : candles)
        {
            // Bind data to the prepared statement
            // Indexes are 1-based
            sqlite3_bind_text(stmt, 1, instrument_key.c_str(), -1, SQLITE_STATIC); // Use SQLITE_STATIC for safety if string might change
            sqlite3_bind_text(stmt, 2, interval.c_str(), -1, SQLITE_STATIC);

            // Format timestamp to IST string matching DB format using updated utility
            std::string timestamp_str = core::utils::timestampToString(candle.timestamp);
            sqlite3_bind_text(stmt, 3, timestamp_str.c_str(), -1, SQLITE_STATIC);

            sqlite3_bind_double(stmt, 4, candle.open);
            sqlite3_bind_double(stmt, 5, candle.high);
            sqlite3_bind_double(stmt, 6, candle.low);
            sqlite3_bind_double(stmt, 7, candle.close);
            sqlite3_bind_int64(stmt, 8, candle.volume);

            // Execute the statement for this row
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE)
            { // SQLITE_DONE is expected for successful INSERT/IGNORE
                core::logging::getLogger()->error("Failed to execute insert step [{}]: {}", rc, sqlite3_errmsg(db_));
                success = false;
                break; // Exit loop on first error
            }
            else
            {
                // Check if a row was actually inserted (not ignored)
                if (sqlite3_changes(db_) > 0)
                {
                    saved_count++;
                }
            }

            // Reset the statement bindings for the next iteration
            rc = sqlite3_reset(stmt);
            if (rc != SQLITE_OK)
            {
                core::logging::getLogger()->error("Failed to reset prepared statement [{}]: {}", rc, sqlite3_errmsg(db_));
                success = false;
                break; // Exit loop on reset error
            }
        }

        // Finalize the statement BEFORE commit/rollback
        sqlite3_finalize(stmt);

        // Commit or rollback transaction
        const char *final_sql = success ? "COMMIT;" : "ROLLBACK;";
        if (!executeSQL(final_sql))
        {
            core::logging::getLogger()->error("Failed to {} transaction for saving candles.", success ? "COMMIT" : "ROLLBACK");
            success = false; // Mark as failure if commit/rollback fails
            // If commit failed, maybe try rollback? But state is likely inconsistent.
            if (final_sql == "COMMIT;")
                executeSQL("ROLLBACK;");
        }
        else
        {
            if (success)
            {
                core::logging::getLogger()->info("Successfully saved {} new candles (duplicates ignored) for {} ({}).", saved_count, instrument_key, interval);
            }
            else
            {
                core::logging::getLogger()->warn("Transaction rolled back due to error during candle save for {} ({}).", instrument_key, interval);
            }
        }

        return success;
    }

} // namespace data
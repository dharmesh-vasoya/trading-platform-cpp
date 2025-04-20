#include "upstox_api_client.hpp"
#include "logging.hpp"
#include "exceptions.hpp"
#include "utils.hpp" 
// Include necessary headers for HTTP client and JSON AFTER adding dependencies
#include <cpr/cpr.h> // Example if using CPR
#include <nlohmann/json.hpp> // Example if using nlohmann/json

#include <stdexcept> // For runtime_error etc.

namespace data {

// Constructor Implementation
UpstoxApiClient::UpstoxApiClient(const std::string& api_key,
                                 const std::string& api_secret,
                                 const std::string& redirect_uri,
                                 const std::string& access_token)
    : api_key_(api_key),
      api_secret_(api_secret),
      redirect_uri_(redirect_uri),
      access_token_(access_token)
{
    core::logging::getLogger()->debug("UpstoxApiClient created.");
    if (access_token_.empty()) {
         core::logging::getLogger()->warn("UpstoxApiClient created without access token.");
         // Might need to implement auth flow later
    }
}


// --- Placeholder Implementations ---

core::TimeSeries<core::Candle> UpstoxApiClient::getHistoricalCandleData(
    const std::string& instrument_key,
    const std::string& interval,
    const std::string& from_date, // YYYY-MM-DD
    const std::string& to_date)   // YYYY-MM-DD
{
core::TimeSeries<core::Candle> candles;
auto logger = core::logging::getLogger();

if (access_token_.empty()) {
    logger->error("Cannot fetch Upstox data: Access token is missing.");
    // In a real app, you might trigger the auth flow here or throw an exception
    return candles; // Return empty
}

// --- Construct the API Endpoint ---
// IMPORTANT: Verify the exact endpoint path and parameter order from Upstox V2 docs!
// This is based on common patterns, but might need adjustment.
// Example: /v2/historical-candle/INSTRUMENT_KEY/INTERVAL/TO_DATE/FROM_DATE
 // URL Encoding is crucial for instrument keys containing special characters like '|'
std::string endpoint = fmt::format("/v2/historical-candle/{}/{}/{}/{}",
                                   cpr::util::urlEncode(instrument_key),
                                   cpr::util::urlEncode(interval),
                                   to_date, // Dates usually don't need encoding
                                   from_date);

std::string full_url = base_url_ + endpoint;
logger->debug("Requesting Upstox URL: {}", full_url);

// --- Prepare Headers ---
cpr::Header headers = {
    {"Accept", "application/json"},
    {"Api-Version", api_version_}, // e.g., "v2"
    {"Authorization", "Bearer " + access_token_}
};

// --- Make the GET Request ---
// Add timeout (e.g., 15 seconds)
cpr::Response response = cpr::Get(cpr::Url{full_url}, headers, cpr::Timeout{15000});

// --- Process the Response ---
logger->debug("Upstox API Response Status: {}, Body size: {}", response.status_code, response.text.length());

if (response.error) {
    logger->error("Upstox API request failed (CPR error): Code={}, Message='{}'",
                  static_cast<int>(response.error.code), response.error.message);
    return candles;
}

if (response.status_code != 200) {
    logger->error("Upstox API request failed: Status Code={}, Body='{}'", response.status_code, response.text);
     if (response.status_code == 401) {
          logger->critical("Upstox API returned 401 Unauthorized. Access token may be invalid or expired.");
          // Potentially clear the token or trigger re-authentication
          access_token_.clear(); // Example: Clear bad token
     }
    return candles;
}

// --- Parse JSON Response ---
try {
    nlohmann::json json_response = nlohmann::json::parse(response.text);

    // Check Upstox specific status within JSON
    if (json_response.contains("status") && json_response["status"] != "success") {
        std::string api_error_msg = json_response.value("message", "Unknown API error message");
        logger->error("Upstox API returned non-success status: Status='{}', Message='{}'",
                      json_response["status"].get<std::string>(), api_error_msg);
        return candles;
    }

    // Navigate to the candle data - **VERIFY THIS PATH AGAINST ACTUAL API RESPONSE**
    if (!json_response.contains("data") || !json_response["data"].contains("candles")) {
        logger->error("Unexpected JSON structure: 'data.candles' not found.");
        return candles;
    }

    const auto& json_candles = json_response["data"]["candles"];
    if (!json_candles.is_array()) {
         logger->error("Unexpected JSON structure: 'data.candles' is not an array.");
         return candles;
    }

    logger->info("Received {} candles from Upstox API.", json_candles.size());

    // --- Convert JSON Candles to core::Candle ---
    for (const auto& json_candle : json_candles) {
        if (!json_candle.is_array() || json_candle.size() < 6) { // Expecting [timestamp, o, h, l, c, v, (oi maybe)]
            logger->warn("Skipping invalid candle data format in JSON array.");
            continue;
        }
        try {
            core::Candle candle;
            // Index 0: Timestamp (string) - Assuming IST format like DB
            std::string ts_str = json_candle[0].get<std::string>();
            candle.timestamp = core::utils::stringToTimestamp(ts_str); // Use our existing parser

            // Index 1-4: OHLC (should be numbers or strings convertible to double)
            candle.open  = json_candle[1].get<double>();
            candle.high  = json_candle[2].get<double>();
            candle.low   = json_candle[3].get<double>();
            candle.close = json_candle[4].get<double>();

            // Index 5: Volume (should be number or string convertible to long long)
            candle.volume = json_candle[5].get<long long>();

            // Index 6: Open Interest (Optional - check if Upstox provides it)
            // If present and needed, add parsing here. Currently ignored.
            candle.open_interest = std::nullopt;

            candles.push_back(candle);

        } catch (const nlohmann::json::exception& e) {
            logger->error("Error parsing individual candle JSON data: {}", e.what());
        } catch (const std::exception& e) {
            logger->error("Error converting candle data: {}", e.what());
        }
    } // End loop through json_candles

} catch (const nlohmann::json::parse_error& e) {
    logger->error("Failed to parse JSON response from Upstox API: {}", e.what());
    logger->error("Response Text (first 500 chars): {}", response.text.substr(0, 500));
} catch (const std::exception& e) {
    logger->error("Unexpected error processing API response: {}", e.what());
}

// Sort just in case API doesn't guarantee order (optional)
// std::sort(candles.begin(), candles.end(), [](const auto& a, const auto& b){ return a.timestamp < b.timestamp; });

logger->debug("Parsed {} candles successfully.", candles.size());
return candles;
}


std::string UpstoxApiClient::performGetRequest(const std::string& endpoint,
                                               const std::vector<std::pair<std::string, std::string>>& params)
{
    core::logging::getLogger()->warn("UpstoxApiClient::performGetRequest not fully implemented yet.");
    // TODO:
    // 1. Construct full URL: base_url_ + "/" + api_version_ + endpoint
    // 2. Create CPR Parameters object from params vector (if any)
    // 3. Set Headers: 'Accept: application/json', 'Api-Version: api_version_', 'Authorization: Bearer access_token_'
    // 4. Make GET request: cpr::Get(cpr::Url{full_url}, cpr::Header{...}, cpr::Parameters{...})
    // 5. Check response.status_code (e.g., 200 is OK)
    // 6. Handle errors (4xx, 5xx, network errors) - throw exception or return empty string?
    // 7. Return response.text on success
    return ""; // Return empty string for now
}


} // namespace data

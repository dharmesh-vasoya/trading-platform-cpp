#pragma once

#include <string>
#include <vector>
#include <optional>
#include "datatypes.hpp" // For TimeSeries, Candle

namespace data {

class UpstoxApiClient {
public:
    // Constructor - Takes API credentials/token
    UpstoxApiClient(const std::string& api_key,
                      const std::string& api_secret,
                      const std::string& redirect_uri,
                      const std::string& access_token = ""); // Access token can be set later

    // --- Authentication Methods (Placeholder) ---
    // bool generateAccessToken(const std::string& auth_code);
    // void setAccessToken(const std::string& token);
    // bool isAccessTokenValid(); // Placeholder

    // --- Data Fetching Methods ---
    core::TimeSeries<core::Candle> getHistoricalCandleData(
        const std::string& instrument_key,
        const std::string& interval,
        const std::string& from_date, // YYYY-MM-DD
        const std::string& to_date    // YYYY-MM-DD
    );

    // --- Other potential methods ---
    // std::vector<InstrumentInfo> getInstrumentMaster(); // Placeholder

private:
    std::string api_key_;
    std::string api_secret_;
    std::string redirect_uri_;
    std::string access_token_;
    std::string api_version_ = "v2"; // Or configurable
    std::string base_url_ = "https://api.upstox.com"; // Or configurable

    // Helper to perform actual HTTP GET request (implementation later)
    std::string performGetRequest(const std::string& endpoint,
                                  const std::vector<std::pair<std::string, std::string>>& params);
};

} // namespace data

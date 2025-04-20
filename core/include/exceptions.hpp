#pragma once

#include <stdexcept>
#include <string>

namespace core {

    class TradingPlatformException : public std::runtime_error {
    public:
        explicit TradingPlatformException(const std::string& message)
            : std::runtime_error(message) {}

        explicit TradingPlatformException(const char* message)
            : std::runtime_error(message) {}
    };

    // Specific exception types
    class ConfigException : public TradingPlatformException {
    public: using TradingPlatformException::TradingPlatformException; };

    class DataLoadException : public TradingPlatformException {
    public: using TradingPlatformException::TradingPlatformException; };

    class ApiRequestException : public TradingPlatformException {
    public: using TradingPlatformException::TradingPlatformException; };

    class IndicatorCalculationException : public TradingPlatformException {
    public: using TradingPlatformException::TradingPlatformException; };

    class StrategyException : public TradingPlatformException {
    public: using TradingPlatformException::TradingPlatformException; };

    class BacktestException : public TradingPlatformException {
    public: using TradingPlatformException::TradingPlatformException; };

} // namespace core
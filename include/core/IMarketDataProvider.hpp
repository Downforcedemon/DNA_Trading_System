#pragma once
#include <string>
#include <vector>
#include <ctime>

class IMarketDataProvider { 
    public:
        virtual ~IMarketDataProvider() = default;

        // connection management
        virtual bool connect() = 0;
        virtual void disconnect() = 0; 
        virtual bool isConnected() const = 0; 

        // Market data subscriptions
        virtual void subscribe(const std::string& symbol) = 0;
        virtual void unsubscribe(const std::string& symbol) = 0;

        // Request previous day's OHLC for pivot calculations (fires onOHLCUpdate when done)
        virtual void requestOHLC(const std::string& symbol) = 0;

        // Provider identification
        virtual std::string getProviderName() const = 0;

        virtual void processMessages() = 0; 
}; 


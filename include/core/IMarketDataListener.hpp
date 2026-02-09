#pragma once
#include <string>
#include <ctime>

class IMarketDataListener { 
    public:
        virtual ~IMarketDataListener() = default; 
        
        // Price update callback
        virtual void onPriceUpdate(const std::string& symbol, double price, time_t timestamp) = 0;
        virtual void onSizeUpdate(const std::string& symbol, int size, time_t timestamp) = 0; 
        virtual void onError(const std::string& symbol, int errorCode, const std::string& errorMsg) = 0;
}; 


#include "core/MarketDataManager.hpp"
#include <algorithm>

MarketDataManager::MarketDataManager()
    : provider_(nullptr), listeners_() {
}


MarketDataManager::~MarketDataManager(){
    if (provider_ && provider_->isConnected()){
        provider_->disconnect();
    }
}

void MarketDataManager::setProvider(std::unique_ptr<IMarketDataProvider> provider) {
    provider_ = std::move(provider); 
} 

IMarketDataProvider* MarketDataManager::getProvider() const {
    return provider_.get(); 
}

void MarketDataManager::addListener(IMarketDataListener* listener){
    listeners_.push_back(listener); 
}

void MarketDataManager::removeListener(IMarketDataListener* listener){
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), listener), listeners_.end());
}

void MarketDataManager::onPriceUpdate(const std::string& symbol, double price, time_t timestamp){
    for (auto* listener: listeners_){
        listener->onPriceUpdate(symbol, price, timestamp);
        }
}

void MarketDataManager::onSizeUpdate(const std::string& symbol, int size, time_t timestamp) {
    for (auto* listener : listeners_) {
        listener-> onSizeUpdate(symbol, size, timestamp);
        }
}

void MarketDataManager::onError(const std::string& symbol, int errorCode, const std::string& errorMsg){
    for (auto* listener : listeners_){
        listener->onError(symbol, errorCode, errorMsg);
    }
}


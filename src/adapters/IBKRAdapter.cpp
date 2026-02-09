#include "adapters/IBKRAdapter.hpp"

IBKRAdapter::IBKRAdapter(IMarketDataListener* listener, const IBKRConfig& config)
    : ibkrConnection_(std::make_unique<IBKRConnection>(this)), listener_(listener), config_(config) {
}

IBKRAdapter::~IBKRAdapter() {
    if (ibkrConnection_) {
        ibkrConnection_->disconnect();
    }
}

bool IBKRAdapter::connect() {
    return ibkrConnection_->connect(config_.host, config_.port, config_.clientId);
}

void IBKRAdapter::disconnect() {
    ibkrConnection_->disconnect();
} 

bool IBKRAdapter::isConnected() const { 
    return ibkrConnection_->isConnected();
}

void IBKRAdapter::subscribe(const std::string& symbol){
    ibkrConnection_->subscribeMarketData(symbol); 
}

void IBKRAdapter::unsubscribe(const std::string& symbol){
    // IBKR doesn't have an unsubscribe method in current implemenation
    // implement later if needed
}

std::string IBKRAdapter::getProviderName() const {
    return "IBKR";
} 

void IBKRAdapter::processMessages() {
    ibkrConnection_->processMessages();
}

// IMarketDataListener callbacks - forward to listener_
void IBKRAdapter::onPriceUpdate(const std::string& symbol, double price, time_t timestamp) {
    listener_->onPriceUpdate(symbol, price, timestamp);
}

void IBKRAdapter::onSizeUpdate(const std::string& symbol, int size, time_t timestamp) {
    listener_->onSizeUpdate(symbol, size, timestamp);
}

void IBKRAdapter::onError(const std::string& symbol, int errorCode, const std::string& errorMsg) {
    listener_->onError(symbol, errorCode, errorMsg);
}

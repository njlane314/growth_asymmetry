#ifndef MARKET_FEED_PROVIDER_H
#define MARKET_FEED_PROVIDER_H

#include <string>
#include <nlohmann/json.hpp>

class MarketFeedProvider {
public:
    virtual ~MarketFeedProvider() = default;
    virtual nlohmann::json fetch(const std::string& endpoint) = 0;
};

#endif
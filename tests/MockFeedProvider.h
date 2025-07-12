#ifndef MOCK_MARKET_FEED_PROVIDER_H
#define MOCK_MARKET_FEED_PROVIDER_H

#include "framework/MarketFeedProvider.h"
#include <map>
#include <string>

class MockFeedProvider : public MarketFeedProvider {
public:
    std::map<std::string, nlohmann::json> responses;

    nlohmann::json fetch(const std::string& endpoint) override {
        if (responses.count(endpoint)) {
            return responses[endpoint];
        }
        return nlohmann::json::object();
    }

    void set_response(const std::string& endpoint, const nlohmann::json& response) {
        responses[endpoint] = response;
    }
};

#endif
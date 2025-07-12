#ifndef POLYGON_FEED_PROVIDER_H
#define POLYGON_FEED_PROVIDER_H

#include "MarketFeedProvider.h"
#include "Config.h"
#include <curl/curl.h>

class PolygonFeedProvider : public MarketFeedProvider {
private:
    const Config& config;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    PolygonFeedProvider(const Config& cfg) : config(cfg) {}

    nlohmann::json fetch(const std::string& endpoint) override {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            std::string url = "https://api.polygon.io" + endpoint + "&apiKey=" + config.api_key;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
            }
        }
        try {
            return nlohmann::json::parse(readBuffer);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }
};

#endif
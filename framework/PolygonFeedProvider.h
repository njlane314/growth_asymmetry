#ifndef POLYGON_FEED_PROVIDER_H
#define POLYGON_FEED_PROVIDER_H

#include "MarketFeedProvider.h"
#include "Config.h"
#include <curl/curl.h>
#include <iostream>

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
        long http_code = 0;

        if (curl) {
            // --- ELEGANT URL CONSTRUCTION ---
            // Check if the endpoint already has query parameters
            char separator = (endpoint.find('?') == std::string::npos) ? '?' : '&';
            std::string url = "https://api.polygon.io" + endpoint + separator + "apiKey=" + config.api_key;
            
            // std::cout << "Fetching URL: " << url << std::endl; // Keep for debugging if needed

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            
            CURLcode res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
            }
            if (http_code != 200) {
                 throw std::runtime_error("API request failed with HTTP status code " + std::to_string(http_code) + ". Response: " + readBuffer);
            }
        }
        try {
            return nlohmann::json::parse(readBuffer);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()) + ". Raw response: " + readBuffer);
        }
    }
};

#endif
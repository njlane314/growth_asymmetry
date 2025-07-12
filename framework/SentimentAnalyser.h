#ifndef SENTIMENT_ANALYSER_H
#define SENTIMENT_ANALYSER_H

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "Config.h"

using json = nlohmann::json;

class SentimentAnalyser {
private:
    const Config& config;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    json fetch_from_api(const std::string& endpoint) {
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
            return json::parse(readBuffer);
        } catch (const json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

    double calculate_volatility(const std::vector<double>& returns) const {
        if (returns.empty()) return 0.0;
        double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        double variance = 0.0;
        for (double r : returns) {
            variance += (r - mean) * (r - mean);
        }
        variance /= returns.size();
        return std::sqrt(variance) * std::sqrt(252);
    }

    double calculate_rsi(const std::vector<double>& returns) const {
        if (returns.size() < config.rsi_period) return 50.0;
        double gain = 0.0, loss = 0.0;
        for (size_t i = 1; i < returns.size(); i++) {
            double change = returns[i] - returns[i-1];
            if (i < config.rsi_period) {
                 if (change > 0) gain += change;
                 else loss -= change;
            }
        }
        gain /= config.rsi_period;
        loss /= config.rsi_period;
        double rs = (loss == 0) ? 100.0 : gain / loss;
        return 100.0 - 100.0 / (1.0 + rs);
    }

public:
    SentimentAnalyser(const Config& cfg) : config(cfg) {}

    std::map<std::string, double> analyse_sentiment(const std::string& ticker) {
        auto now = std::chrono::system_clock::now();
        auto to_date = now;
        auto from_date = now - std::chrono::hours(24 * config.sentiment_lookback_days);
        std::time_t to_time = std::chrono::system_clock::to_time_t(to_date);
        std::time_t from_time = std::chrono::system_clock::to_time_t(from_date);
        std::stringstream to_ss, from_ss;
        to_ss << std::put_time(std::localtime(&to_time), "%Y-%m-%d");
        from_ss << std::put_time(std::localtime(&from_time), "%Y-%m-%d");

        std::string endpoint = "/v2/aggs/ticker/" + ticker + "/range/1/day/" + from_ss.str() + "/" + to_ss.str();

        std::map<std::string, double> metrics;
        try {
            json data = fetch_from_api(endpoint);
            std::vector<double> returns;
            if (!data.empty() && !data["results"].empty()) {
                for (const auto& day : data["results"]) {
                    double open = day.value("o", 0.0);
                    double close = day.value("c", 0.0);
                    if (open > 0) returns.push_back((close - open) / open);
                }
            }
            metrics["volatility"] = calculate_volatility(returns);
            metrics["rsi"] = calculate_rsi(returns);
            metrics["beta"] = config.default_beta;
            metrics["sentiment_score"] = (50.0 - metrics["rsi"] * 0.3) + (metrics["volatility"] * -0.2) + (metrics["beta"] * 0.1) + 0.6;
        } catch (const std::runtime_error& e) {
            std::cerr << "Error fetching sentiment for " << ticker << ": " << e.what() << std::endl;
            return {};
        }
        return metrics;
    }
};

#endif
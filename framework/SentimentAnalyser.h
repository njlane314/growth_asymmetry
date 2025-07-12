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
#include <nlohmann/json.hpp>

#include "Config.h"
#include "MarketFeedProvider.h"

using json = nlohmann::json;

class SentimentAnalyser {
private:
    const Config& config;
    MarketFeedProvider& feed_provider;

    json fetch_from_api(const std::string& endpoint) {
        return feed_provider.fetch(endpoint);
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

    double calculate_rsi(const std::vector<double>& prices) const {
        if (prices.size() <= config.rsi_period) return 50.0;

        double gain = 0.0;
        double loss = 0.0;

        for (size_t i = 1; i <= config.rsi_period; ++i) {
            double change = prices[i] - prices[i - 1];
            if (change > 0) {
                gain += change;
            } else {
                loss -= change;
            }
        }

        gain /= config.rsi_period;
        loss /= config.rsi_period;

        for (size_t i = config.rsi_period + 1; i < prices.size(); ++i) {
            double change = prices[i] - prices[i - 1];
            if (change > 0) {
                gain = (gain * (config.rsi_period - 1) + change) / config.rsi_period;
                loss = (loss * (config.rsi_period - 1)) / config.rsi_period;
            } else {
                loss = (loss * (config.rsi_period - 1) - change) / config.rsi_period;
                gain = (gain * (config.rsi_period - 1)) / config.rsi_period;
            }
        }

        if (loss == 0) return 100.0;

        double rs = gain / loss;
        return 100.0 - (100.0 / (1.0 + rs));
    }

public:
    SentimentAnalyser(const Config& cfg, MarketFeedProvider& provider) 
        : config(cfg), feed_provider(provider) {}

    std::map<std::string, double> analyse_sentiment(const std::string& ticker) {
        auto now = std::chrono::system_clock::now();
        auto to_date = now;
        auto from_date = now - std::chrono::hours(24 * config.sentiment_lookback_days);
        std::time_t to_time = std::chrono::system_clock::to_time_t(to_date);
        std::time_t from_time = std::chrono::system_clock::to_time_t(from_date);
        std::stringstream to_ss, from_ss;
        to_ss << std::put_time(std::localtime(&to_time), "%Y-%m-%d");
        from_ss << std::put_time(std::localtime(&from_time), "%Y-%m-%d");
        return analyse_sentiment_for_range(ticker, from_ss.str(), to_ss.str());
    }

    std::map<std::string, double> analyse_sentiment_for_range(const std::string& ticker, const std::string& from, const std::string& to) {
        std::string endpoint = "/v2/aggs/ticker/" + ticker + "/range/1/day/" + from + "/" + to;

        std::map<std::string, double> metrics;
        try {
            json data = fetch_from_api(endpoint);
            std::vector<double> prices;
            if (!data.empty() && data.contains("results") && !data["results"].is_null()) {
                for (const auto& day : data["results"]) {
                    prices.push_back(day.value("c", 0.0));
                }
            } else {
                return {};
            }

            if (prices.empty()) return {};
            
            std::vector<double> returns;
            for(size_t i = 1; i < prices.size(); ++i) {
                if (prices[i-1] > 0) {
                    returns.push_back((prices[i] - prices[i-1]) / prices[i-1]);
                }
            }

            metrics["volatility"] = calculate_volatility(returns);
            metrics["rsi"] = calculate_rsi(prices);
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
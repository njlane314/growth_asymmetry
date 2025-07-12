#ifndef SENTIMENT_ANALYSER_H
#define SENTIMENT_ANALYSER_H

#include <string>
#include <map>
#include <curl/curl.h>
#include "json.hpp"
#include <cmath>
#include <numeric>
#include <vector>

using json = nlohmann::json;

/* Quantitative Sentiment Analysis for Growth Stocks
This class computes sentiment scores using volatility, RSI, and beta from historical returns,
providing a numerical proxy for market mood without news. Volatility measures price fluctuation risk,
RSI detects overbought/oversold conditions, and beta gauges market sensitivity.

Volatility Calculation
vol = sqrt( variance of daily returns ) * sqrt(252)

variance = (1/n) sum (r_i - mean_r)^2, where r_i = ln(close / open). Explanation: Annualizes daily fluctuation to capture risk in growth phases.
RSI Calculation
RSI = 100 - 100 / (1 + RS), RS = average_gain / average_loss over period (14 days)

average_gain: Sum of positive changes / period.
average_loss: Sum of absolute negative changes / period. Explanation: Indicates momentum; 30-70 range favors balanced sentiment for sustainable growth.
beta: Hardcoded or fetched (1.2 default). Explanation: Measures stock volatility relative to market, biasing toward stable growers.
Sentiment Score
sentiment_score = (50 - RSI * 0.3) + (vol * -0.2) + (beta * 0.1) + 0.6

Explanation: Balances momentum (RSI penalty for extremes), risk (vol subtraction), and market alignment (beta addition),
normalized to favor stocks with positive, rising sentiment in scaling phases.
*/

class SentimentAnalyser {
private:
    std::string api_key;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    json fetch_from_api(const std::string& endpoint) {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            std::string url = "https://api.polygon.io" + endpoint + "&apiKey=" + api_key;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                return json();
            }
        }
        try {
            return json::parse(readBuffer);
        } catch (...) {
            return json();
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

    double calculate_rsi(const std::vector<double>& returns, int period = 14) const {
        if (returns.size() < period) return 50.0;
        double gain = 0.0, loss = 0.0;
        for (int i = 1; i < period; i++) {
            double change = returns[i] - returns[i-1];
            if (change > 0) gain += change;
            else loss -= change;
        }
        gain /= period;
        loss /= period;
        double rs = (loss == 0) ? 100.0 : gain / loss;
        return 100.0 - 100.0 / (1.0 + rs);
    }

public:
    SentimentAnalyser(const std::string& key) : api_key(key) {}

    std::map<std::string, double> analyse_sentiment(const std::string& ticker) {
        std::string endpoint = "/v2/aggs/ticker/" + ticker + "/range/1/day/2025-06-12/2025-07-12";
        json data = fetch_from_api(endpoint);
        std::vector<double> returns;
        if (!data.empty() && !data["results"].empty()) {
            for (const auto& day : data["results"]) {
                double open = day.value("o", 0.0);
                double close = day.value("c", 0.0);
                if (open > 0) returns.push_back((close - open) / open);
            }
        }
        std::map<std::string, double> metrics;
        metrics["volatility"] = calculate_volatility(returns);
        metrics["rsi"] = calculate_rsi(returns);
        metrics["beta"] = 1.2;
        metrics["sentiment_score"] = (50.0 - metrics["rsi"] * 0.3) + (metrics["volatility"] * -0.2) + (metrics["beta"] * 0.1) + 0.6;
        return metrics;
    }
};

#endif
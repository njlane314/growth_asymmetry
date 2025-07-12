#ifndef FUNDAMENTALS_ANALYZER_H
#define FUNDAMENTALS_ANALYZER_H

#include <string>
#include <map>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

class FundamentalsAnalyzer {
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

public:
    FundamentalsAnalyzer(const std::string& key) : api_key(key) {}

    std::map<std::string, double> analyze_fundamentals(const std::string& ticker) {
        std::string endpoint = "/vX/reference/financials?ticker=" + ticker + "&filing_date.gte=2024-01-01&limit=1&timeframe=annual";
        json data = fetch_from_api(endpoint);
        std::map<std::string, double> metrics;
        if (!data.empty() && !data["results"].empty()) {
            json financials = data["results"][0]["financials"];
            double revenue_current = financials["income_statement"]["revenues"].value("value", 0.0);
            metrics["revenue_growth"] = 0.20;
            metrics["roe"] = financials["profitability"]["return_on_equity_ttm"].value("value", 0.0);
            metrics["debt_equity"] = financials["balance_sheet"]["total_debt_to_equity_ratio"].value("value", 0.0);
            metrics["fcf_yield"] = financials["cash_flow_statement"]["free_cash_flow"].value("value", 0.0) / financials["balance_sheet"]["market_capitalization"].value("value", 1.0);
            metrics["profit_margin"] = financials["income_statement"]["net_profit_margin_ttm"].value("value", 0.0);
            metrics["pe_ratio"] = financials["valuation"]["price_to_earnings_ratio_ttm"].value("value", 0.0);
            metrics["peg_ratio"] = financials["valuation"]["price_earnings_to_growth_ratio_ttm"].value("value", 0.0);
            metrics["market_cap"] = financials["balance_sheet"]["market_capitalization"].value("value", 0.0) / 1e9;
            metrics["fundamentals_score"] = (metrics["revenue_growth"] * 0.3) + (metrics["roe"] * 0.2) + (metrics["fcf_yield"] * 0.2) + (metrics["profit_margin"] * 0.15) + (1.0 / (metrics["pe_ratio"] + 1e-6) * 0.1) + (1.0 / (metrics["debt_equity"] + 1e-6) * 0.05);
        }
        return metrics;
    }
};

#endif
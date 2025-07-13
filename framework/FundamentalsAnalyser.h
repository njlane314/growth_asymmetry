#ifndef FUNDAMENTALS_ANALYSER_H
#define FUNDAMENTALS_ANALYSER_H

#include <string>
#include <map>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>

#include "Config.h"
#include "MarketFeedProvider.h"

using json = nlohmann::json;

class FundamentalsAnalyser {
private:
    const Config& config;
    MarketFeedProvider& feed_provider;

    json fetch_from_api(const std::string& endpoint) {
        return feed_provider.fetch(endpoint);
    }

    /**
     * @brief Safely retrieves a numeric value from a nested JSON object.
     *
     * This function uses a JSON pointer to navigate the JSON structure. It checks
     * for the existence of the path and ensures the value is not null before
     * returning it. If the path is invalid or the value is null, it returns a
     * default value of 0.0.
     *
     * @param financials_json The JSON object to query.
     * @param pointer The JSON pointer string (e.g., "/income_statement/revenues/value").
     * @return The extracted double value, or 0.0 if not found or null.
     */
    double get_financial_value(const json& financials_json, const std::string& pointer_str) const {
        try {
            const json::json_pointer ptr(pointer_str);
            if (financials_json.contains(ptr) && !financials_json.at(ptr).is_null()) {
                return financials_json.at(ptr).get<double>();
            }
        } catch (const json::parse_error& e) {
            // This can happen if the pointer string is malformed, though unlikely here.
            std::cerr << "Warning: JSON pointer parse error: " << e.what() << std::endl;
        }
        return 0.0;
    }


public:
    FundamentalsAnalyser(const Config& cfg, MarketFeedProvider& provider)
        : config(cfg), feed_provider(provider) {}

    std::map<std::string, double> analyze_fundamentals(const std::string& ticker) {
        std::string endpoint_current = "/vX/reference/financials?ticker=" + ticker + "&filing_date.gte=2024-01-01&limit=1&timeframe=annual";
        std::string endpoint_prior = "/vX/reference/financials?ticker=" + ticker + "&filing_date.gte=2023-01-01&filing_date.lt=2024-01-01&limit=1&timeframe=annual";

        std::map<std::string, double> metrics;
        try {
            json data_current = fetch_from_api(endpoint_current);
            json data_prior = fetch_from_api(endpoint_prior);

            // --- ROBUSTNESS CHECKS (kept for clarity) ---
            if (data_current.empty() || !data_current.contains("results") || data_current["results"].empty() ||
                data_prior.empty() || !data_prior.contains("results") || data_prior["results"].empty()) {
                std::cerr << "Warning: Could not retrieve full financials for " << ticker << ". Skipping." << std::endl;
                return {};
            }

            const auto& financials_current = data_current["results"][0]["financials"];
            const auto& financials_prior = data_prior["results"][0]["financials"];

            if (financials_current.is_null() || financials_prior.is_null()) {
                 std::cerr << "Warning: Financials object missing for " << ticker << ". Skipping." << std::endl;
                return {};
            }

            // --- Elegant Value Extraction ---
            double revenue_current = get_financial_value(financials_current, "/income_statement/revenues/value");
            double revenue_prior = get_financial_value(financials_prior, "/income_statement/revenues/value");

            metrics["revenue_growth"] = (revenue_prior > 0) ? ((revenue_current / revenue_prior) - 1.0) : 0.0;
            metrics["roe"] = get_financial_value(financials_current, "/financial_ratios/return_on_equity/value");
            metrics["debt_equity"] = get_financial_value(financials_current, "/balance_sheet/total_debt_to_equity_ratio/value");
            metrics["profit_margin"] = get_financial_value(financials_current, "/income_statement/net_profit_margin_ttm/value");
            metrics["pe_ratio"] = get_financial_value(financials_current, "/valuation/price_to_earnings_ratio_ttm/value");
            metrics["peg_ratio"] = get_financial_value(financials_current, "/valuation/price_earnings_to_growth_ratio_ttm/value");
            metrics["market_cap"] = get_financial_value(financials_current, "/market_cap/value") / 1e9; // Convert to billions

            double fcf = get_financial_value(financials_current, "/cash_flow_statement/free_cash_flow/value");
            double mcap_raw = get_financial_value(financials_current, "/market_cap/value");
            metrics["fcf_yield"] = (mcap_raw > 0) ? (fcf / mcap_raw) : 0.0;
            
            // --- Scoring Logic (unchanged) ---
            metrics["fundamentals_score"] = (metrics["revenue_growth"] * 0.3) + (metrics["roe"] * 0.2) + (metrics["fcf_yield"] * 0.2) + (metrics["profit_margin"] * 0.15) + (1.0 / (metrics["pe_ratio"] + 1e-6) * 0.1) + (1.0 / (metrics["debt_equity"] + 1e-6) * 0.05);

        } catch (const std::exception& e) {
            std::cerr << "Error during fundamental analysis for " << ticker << ": " << e.what() << std::endl;
            return {};
        }
        return metrics;
    }
};

#endif
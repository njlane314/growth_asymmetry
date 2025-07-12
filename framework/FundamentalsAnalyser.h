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

            if (data_current.empty() || data_current["results"].empty() || data_prior.empty() || data_prior["results"].empty()) {
                return metrics;
            }

            json financials_current = data_current["results"][0]["financials"];
            json financials_prior = data_prior["results"][0]["financials"];

            double revenue_current = financials_current["income_statement"]["revenues"].value("value", 0.0);
            double revenue_prior = financials_prior["income_statement"]["revenues"].value("value", 0.0);

            if (revenue_prior > 0) {
                metrics["revenue_growth"] = (revenue_current / revenue_prior) - 1.0;
            } else {
                metrics["revenue_growth"] = 0.0;
            }

            metrics["roe"] = financials_current["financial_ratios"]["return_on_equity"].value("value", 0.0);
            metrics["debt_equity"] = financials_current["balance_sheet"]["total_debt_to_equity_ratio"].value("value", 0.0);
            metrics["fcf_yield"] = financials_current["cash_flow_statement"]["free_cash_flow"].value("value", 0.0) / financials_current["market_cap"].value("value", 1.0);
            metrics["profit_margin"] = financials_current["income_statement"]["net_profit_margin_ttm"].value("value", 0.0);
            metrics["pe_ratio"] = financials_current["valuation"]["price_to_earnings_ratio_ttm"].value("value", 0.0);
            metrics["peg_ratio"] = financials_current["valuation"]["price_earnings_to_growth_ratio_ttm"].value("value", 0.0);
            metrics["market_cap"] = financials_current["market_cap"].value("value", 0.0) / 1e9;
            metrics["fundamentals_score"] = (metrics["revenue_growth"] * 0.3) + (metrics["roe"] * 0.2) + (metrics["fcf_yield"] * 0.2) + (metrics["profit_margin"] * 0.15) + (1.0 / (metrics["pe_ratio"] + 1e-6) * 0.1) + (1.0 / (metrics["debt_equity"] + 1e-6) * 0.05);

        } catch (const std::runtime_error& e) {
            std::cerr << "Error fetching fundamentals for " << ticker << ": " << e.what() << std::endl;
            return {};
        }
        return metrics;
    }
};

#endif
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

#include "tests/MockFeedProvider.h"
#include "framework/FundamentalsAnalyser.h"
#include "framework/SentimentAnalyser.h"
#include "framework/Config.h"

struct SentimentAnalyserFixture {
    Config config;
    MockFeedProvider mock_provider;
    SentimentAnalyser analyser;

    SentimentAnalyserFixture() : analyser(config, mock_provider) {}
};

TEST_CASE_METHOD(SentimentAnalyserFixture, "SentimentAnalyser calculates RSI correctly", "[sentiment]") {
    nlohmann::json mock_response = R"({
        "results": [
            {"c": 100}, {"c": 101}, {"c": 102}, {"c": 103}, {"c": 104},
            {"c": 105}, {"c": 106}, {"c": 107}, {"c": 108}, {"c": 109},
            {"c": 110}, {"c": 111}, {"c": 112}, {"c": 113}, {"c": 114}
        ]
    })"_json;
    
    std::string from_date = "2023-01-01";
    std::string to_date = "2023-03-31";
    std::string endpoint = "/v2/aggs/ticker/AAPL/range/1/day/" + from_date + "/" + to_date;
    mock_provider.set_response(endpoint, mock_response);

    auto metrics = analyser.analyse_sentiment_for_range("AAPL", from_date, to_date);

    REQUIRE(metrics.count("rsi"));
    CHECK(metrics["rsi"] == Approx(100.0));
}

TEST_CASE_METHOD(SentimentAnalyserFixture, "SentimentAnalyser returns empty map on fetch error", "[sentiment]") {
    std::string from_date = "2023-01-01";
    std::string to_date = "2023-03-31";
    std::string endpoint = "/v2/aggs/ticker/FAIL/range/1/day/" + from_date + "/" + to_date;
    mock_provider.set_response(endpoint, nlohmann::json::object());

    auto metrics = analyser.analyse_sentiment_for_range("FAIL", from_date, to_date);

    REQUIRE(metrics.empty());
}

struct FundamentalsAnalyserFixture {
    Config config;
    MockFeedProvider mock_provider;
    FundamentalsAnalyser analyser;

    FundamentalsAnalyserFixture() : analyser(config, mock_provider) {
        nlohmann::json current_financials = R"({
            "results": [{
                "financials": {
                    "income_statement": {
                        "revenues": {"value": 200000},
                        "net_profit_margin_ttm": {"value": 0.15}
                    },
                    "financial_ratios": {
                        "return_on_equity": {"value": 0.25}
                    },
                    "balance_sheet": {
                        "total_debt_to_equity_ratio": {"value": 0.5}
                    },
                    "cash_flow_statement": {
                        "free_cash_flow": {"value": 50000}
                    },
                    "market_cap": {
                        "value": 1000000
                    },
                    "valuation": {
                        "price_to_earnings_ratio_ttm": {"value": 20.0},
                        "price_earnings_to_growth_ratio_ttm": {"value": 1.5}
                    }
                }
            }]
        })"_json;

        nlohmann::json prior_financials = R"({
            "results": [{
                "financials": {
                    "income_statement": {"revenues": {"value": 100000}}
                }
            }]
        })"_json;

        std::string current_endpoint = "/vX/reference/financials?ticker=GOOD&filing_date.gte=2024-01-01&limit=1&timeframe=annual";
        std::string prior_endpoint = "/vX/reference/financials?ticker=GOOD&filing_date.gte=2023-01-01&filing_date.lt=2024-01-01&limit=1&timeframe=annual";
        
        mock_provider.set_response(current_endpoint, current_financials);
        mock_provider.set_response(prior_endpoint, prior_financials);
    }
};

TEST_CASE_METHOD(FundamentalsAnalyserFixture, "FundamentalsAnalyser calculates revenue growth", "[fundamentals]") {
    auto metrics = analyser.analyze_fundamentals("GOOD");

    REQUIRE(metrics.count("revenue_growth"));
    CHECK(metrics["revenue_growth"] == Approx(1.0));
}

TEST_CASE_METHOD(FundamentalsAnalyserFixture, "FundamentalsAnalyser correctly parses ROE and FCF Yield", "[fundamentals]") {
    auto metrics = analyser.analyze_fundamentals("GOOD");

    REQUIRE(metrics.count("roe"));
    CHECK(metrics["roe"] == Approx(0.25));

    REQUIRE(metrics.count("fcf_yield"));
    CHECK(metrics["fcf_yield"] == Approx(0.05));
}

#ifndef UNIVERSE_BUILDER_H
#define UNIVERSE_BUILDER_H

#include "Universe.h"
#include "FundamentalsAnalyser.h"
#include "QuantitativeSentimentAnalyser.h"
#include "GrowthForecast.h"
#include "UniverseCache.h"
#include <string>
#include <vector>
#include <algorithm>

class UniverseBuilder {
private:
    std::string api_key;
    std::vector<std::string> candidates;
    FundamentalsAnalyser fundamentals_analyser;
    QuantitativeSentimentAnalyser sentiment_analyser;
    GrowthForecast growth_forecast;

public:
    UniverseBuilder(const std::string& key, const std::vector<std::string>& initial_candidates)
        : api_key(key), candidates(initial_candidates), fundamentals_analyser(key), sentiment_analyser(key) {}

    Universe build(int top_n = 50, const std::string& prior_filename = "prior_universe.csv", const std::string& current_filename = "current_universe.csv") {
        Universe universe;
        universe.load_prior(prior_filename);
        std::vector<Stock> prior = universe.get_stocks();
        std::vector<Stock> new_stocks;
        for (const auto& ticker : candidates) {
            auto fund_metrics = fundamentals_analyser.analyse_fundamentals(ticker);
            if (fund_metrics.empty()) continue;
            auto sent_metrics = sentiment_analyser.analyse_sentiment(ticker);
            Stock s;
            s.ticker = ticker;
            s.revenue_growth = fund_metrics["revenue_growth"];
            s.roe = fund_metrics["roe"];
            s.debt_equity = fund_metrics["debt_equity"];
            s.fcf_yield = fund_metrics["fcf_yield"];
            s.profit_margin = fund_metrics["profit_margin"];
            s.pe_ratio = fund_metrics["pe_ratio"];
            s.peg_ratio = fund_metrics["peg_ratio"];
            s.market_cap = fund_metrics["market_cap"];
            s.forecasted_growth = growth_forecast.forecast(s);
            s.score = s.forecasted_growth * 0.5 + fund_metrics["fundamentals_score"] * 0.3 + sent_metrics["sentiment_score"] * 0.2;
            new_stocks.push_back(s);
        }
        std::sort(new_stocks.begin(), new_stocks.end(), [](const Stock& a, const Stock& b) {
            return a.score > b.score;
        });
        if (new_stocks.size() > static_cast<size_t>(top_n)) {
            new_stocks.resize(top_n);
        }
        auto changes = universe.compute_changes(prior);
        universe.set_stocks(new_stocks);
        universe.save_current(current_filename);
        return universe;
    }
};

#endif
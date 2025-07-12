#ifndef FACTOR_SCREENER_H
#define FACTOR_SCREENER_H

#include "InvestableUniverse.h"
#include "FundamentalsAnalyser.h"
#include "SentimentAnalyser.h"
#include "GrowthForecast.h"
#include "Config.h"
#include "PolygonFeedProvider.h" 
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

class FactorScreener {
private:
    const Config& config;
    PolygonFeedProvider feed_provider; 
    FundamentalsAnalyser fundamentals_analyser;
    SentimentAnalyser sentiment_analyser;
    GrowthForecast growth_forecast;

public:
    FactorScreener(const Config& cfg)
        : config(cfg),
          feed_provider(cfg),
          fundamentals_analyser(cfg, feed_provider),
          sentiment_analyser(cfg, feed_provider),
          growth_forecast(cfg) {}

    InvestableUniverse build() {
        InvestableUniverse universe;
        universe.load_prior(config.prior_universe_path);
        const auto& prior = universe.get_stocks();
        std::vector<Stock> new_stocks;

        for (const auto& ticker : config.initial_candidates) {
            std::cout << "Processing: " << ticker << std::endl;
            auto fund_metrics = fundamentals_analyser.analyze_fundamentals(ticker);
            if (fund_metrics.empty()) {
                std::cerr << "Skipping " << ticker << " due to fundamental data issues." << std::endl;
                continue;
            }

            auto sent_metrics = sentiment_analyser.analyse_sentiment(ticker);
             if (sent_metrics.empty()) {
                std::cerr << "Skipping " << ticker << " due to sentiment data issues." << std::endl;
                continue;
            }

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
            s.forecasted_growth = growth_forecast.forecast(s, 100.0);
            s.score = s.forecasted_growth * 0.5 + fund_metrics["fundamentals_score"] * 0.3 + sent_metrics["sentiment_score"] * 0.2;
            new_stocks.push_back(s);
        }

        std::sort(new_stocks.begin(), new_stocks.end(), [](const Stock& a, const Stock& b) {
            return a.score > b.score;
        });

        if (new_stocks.size() > static_cast<size_t>(config.top_n_stocks)) {
            new_stocks.resize(config.top_n_stocks);
        }

        auto changes = universe.compute_changes(prior);
        for(const auto& change : changes) {
            std::cout << "Change: " << change << std::endl;
        }

        universe.set_stocks(new_stocks);
        universe.save_current(config.current_universe_path);
        return universe;
    }
};

#endif
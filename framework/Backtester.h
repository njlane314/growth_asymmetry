#ifndef BACKTESTER_H
#define BACKTESTER_H

#include "Universe.h"
#include "Portfolio.h"
#include "DataHandler.h"
#include "Performance.h"
#include "UniverseBuilder.h"
#include "PortfolioAllocator.h"
#include "AlphaDecayModel.h"
#include "MarketSentiment.h"
#include <iostream>
#include <string>
#include <vector>

class Backtester {
public:
    Backtester(const Config& config, double initial_capital)
        : config(config),
          data_handler(config),
          portfolio(initial_capital, data_handler),
          performance(),
          universe_builder(config),
          portfolio_allocator(config),
          alpha_decay_model(),
          market_sentiment() {}

    void run() {
        data_handler.load_all_data();
        const auto& dates = data_handler.get_all_dates();
        Universe current_universe;

        for (const auto& date : dates) {
            std::cout << "Backtesting for date: " << date << std::endl;
            data_handler.set_current_date(date);

            Universe new_universe = universe_builder.build();

            if (alpha_decay_model.has_alpha_decayed(new_universe, current_universe, market_sentiment)) {
                std::cout << "--- Alpha has decayed. Rebalancing to capture new opportunities. ---" << std::endl;
                auto weights = portfolio_allocator.allocate(new_universe, portfolio.get_weights());
                portfolio.rebalance(new_universe.get_stocks(), weights);
                current_universe = new_universe;
            } else {
                std::cout << "--- Holding current portfolio. Alpha signal remains stable. ---" << std::endl;
            }
            
            performance.update(portfolio.get_total_value());
        }

        performance.print_summary();
    }

private:
    const Config& config;
    DataHandler data_handler;
    Portfolio portfolio;
    Performance performance;
    UniverseBuilder universe_builder;
    PortfolioAllocator portfolio_allocator;
    AlphaDecayModel alpha_decay_model;
    MarketSentiment market_sentiment;
};

#endif
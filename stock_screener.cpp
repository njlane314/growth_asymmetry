#include <iostream>
#include "framework/Config.h"
#include "framework/FactorScreener.h"
#include "framework/InvestableUniverse.h" 

int main() {
    Config config;
    config.api_key = "YOUR_API_KEY_HERE";
    config.initial_candidates = {"AAPL", "MSFT", "GOOGL", "AMZN", "NVDA", "TSLA", "META"};
    config.prior_universe_path = "prior_universe.csv";
    config.current_universe_path = "current_universe.csv";

    FactorScreener screener(config);

    std::cout << "Starting the stock screening process..." << std::endl;

    try {
        InvestableUniverse universe = screener.build(); 

        std::cout << "\nScreening process completed successfully!" << std::endl;
        std::cout << "The results have been saved to: " << config.current_universe_path << std::endl;

        const auto& stocks = universe.get_stocks();
        std::cout << "\nTop " << stocks.size() << " stocks in the universe:" << std::endl;
        for (const auto& stock : stocks) {
            std::cout << "  - Ticker: " << stock.ticker << ", Score: " << stock.score << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "\nAn error occurred during the screening process: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
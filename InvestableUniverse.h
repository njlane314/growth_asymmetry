#ifndef UNIVERSE_H
#define UNIVERSE_H

#include "Stock.h"
#include "UniverseCache.h"
#include <vector>
#include <algorithm>

class InvestableUniverse {
private:
    std::vector<Stock> current_stocks;
    UniverseCache cache;

public:
    InvestableUniverse() {}

    const std::vector<Stock>& get_stocks() const {
        return current_stocks;
    }

    void set_stocks(const std::vector<Stock>& stocks) {
        current_stocks = stocks;
    }

    void load_prior(const std::string& filename) {
        current_stocks = cache.load_prior(filename);
    }

    void save_current(const std::string& filename) const {
        cache.save_current(current_stocks, filename);
    }

    std::vector<std::string> compute_changes(const std::vector<Stock>& prior) const {
        return cache.compute_changes(current_stocks, prior);
    }
};

#endif
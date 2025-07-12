#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "DataHandler.h"
#include "Stock.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <numeric>

class Portfolio {
public:
    Portfolio(double initial_capital, DataHandler& data_handler)
        : cash(initial_capital), data_handler(data_handler) {}

    void rebalance(const std::vector<Stock>& stocks, const std::vector<double>& new_weights) {
        std::cout << "Rebalancing portfolio..." << std::endl;
        double total_value = get_total_value();
        for(size_t i = 0; i < stocks.size(); ++i) {
            double target_value = total_value * new_weights[i];
            double current_price = data_handler.get_price(stocks[i].ticker);
            int target_shares = (current_price > 0) ? (target_value / current_price) : 0;
            holdings[stocks[i].ticker] = target_shares;
        }
    }

    double get_total_value() const {
        double total_value = cash;
        for (const auto& pair : holdings) {
            total_value += pair.second * data_handler.get_price(pair.first);
        }
        return total_value;
    }
    
    std::vector<double> get_weights() const {
        std::vector<double> weights;
        double total_value = get_total_value();
        if (total_value == 0) return weights;
        for (const auto& pair : holdings) {
            weights.push_back((pair.second * data_handler.get_price(pair.first)) / total_value);
        }
        return weights;
    }

private:
    double cash;
    std::map<std::string, int> holdings;
    DataHandler& data_handler;
};

#endif
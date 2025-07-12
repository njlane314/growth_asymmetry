#ifndef STOCK_H
#define STOCK_H

#include <string>

struct Stock {
    std::string ticker;
    double revenue_growth = 0.0;
    double roe = 0.0;
    double debt_equity = 0.0;
    double fcf_yield = 0.0;
    double profit_margin = 0.0;
    double pe_ratio = 0.0;
    double peg_ratio = 0.0;
    double market_cap = 0.0;
    double forecasted_growth = 0.0;
    double score = 0.0;
};

#endif
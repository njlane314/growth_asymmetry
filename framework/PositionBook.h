#ifndef POSITION_BOOK_H
#define POSITION_BOOK_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "Stock.h"

class PositionBook {
public:
    std::vector<Stock> load_prior(const std::string& filename) const {
        std::vector<Stock> prior;
        std::ifstream file(filename);
        if (!file) return prior;
        std::string line;
        std::getline(file, line);
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            Stock s;
            std::string token;
            std::getline(ss, token, ','); s.ticker = token;
            std::getline(ss, token, ','); s.revenue_growth = std::stod(token);
            std::getline(ss, token, ','); s.roe = std::stod(token);
            std::getline(ss, token, ','); s.debt_equity = std::stod(token);
            std::getline(ss, token, ','); s.fcf_yield = std::stod(token);
            std::getline(ss, token, ','); s.profit_margin = std::stod(token);
            std::getline(ss, token, ','); s.pe_ratio = std::stod(token);
            std::getline(ss, token, ','); s.peg_ratio = std::stod(token);
            std::getline(ss, token, ','); s.market_cap = std::stod(token);
            std::getline(ss, token, ','); s.forecasted_growth = std::stod(token);
            std::getline(ss, token, ','); s.score = std::stod(token);
            prior.push_back(s);
        }
        return prior;
    }

    void save_current(const std::vector<Stock>& universe, const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) return;
        file << "Ticker,Revenue Growth,ROE,Debt/Equity,FCF Yield,Profit Margin,PE Ratio,PEG Ratio,Market Cap,Forecasted Growth,Score\n";
        for (const auto& s : universe) {
            file << s.ticker << "," << s.revenue_growth << "," << s.roe << "," << s.debt_equity << "," 
                 << s.fcf_yield << "," << s.profit_margin << "," << s.pe_ratio << "," << s.peg_ratio << "," 
                 << s.market_cap << "," << s.forecasted_growth << "," << s.score << "\n";
        }
    }

    std::vector<std::string> compute_changes(const std::vector<Stock>& current, const std::vector<Stock>& prior) const {
        std::vector<std::string> changes;
        for (const auto& s : current) {
            bool found = false;
            for (const auto& p : prior) {
                if (s.ticker == p.ticker) {
                    found = true;
                    break;
                }
            }
            if (!found) changes.push_back("Added: " + s.ticker);
        }
        for (const auto& p : prior) {
            bool found = false;
            for (const auto& s : current) {
                if (p.ticker == s.ticker) {
                    found = true;
                    break;
                }
            }
            if (!found) changes.push_back("Removed: " + p.ticker);
        }
        return changes;
    }
};

#endif
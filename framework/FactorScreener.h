#ifndef FACTOR_SCREENER_H
#define FACTOR_SCREENER_H

#include "InvestableUniverse.h"
#include "Config.h"
#include "FinancialProcessor.h"

#include <sqlite3.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>
#include <cmath>

class FactorScreener {
private:
    const Config& config;
    FinancialProcessor db_processor;
    std::map<std::string, int> ticker_to_cik;
    std::map<int, std::string> cik_to_ticker;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    void load_ticker_to_cik() {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string response;
            curl_easy_setopt(curl, CURLOPT_URL, "https://www.sec.gov/files/company_tickers.json");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res == CURLE_OK) {
                auto j = nlohmann::json::parse(response);
                for (auto& el : j.items()) {
                    auto obj = el.value();
                    int cik = obj["cik_str"].get<int>();
                    std::string ticker = obj["ticker"].get<std::string>();
                    ticker_to_cik[ticker] = cik;
                    cik_to_ticker[cik] = ticker;
                }
            } else {
                std::cerr << "Failed to fetch CIK mapping: " << curl_easy_strerror(res) << std::endl;
            }
        }
    }

    double calculate_cagr(const std::map<std::string, double>& time_series) {
        if (time_series.size() < 5) return 0.0;
        auto first = *time_series.begin();
        auto last = *time_series.rbegin();
        if (first.second <= 0.0) return 0.0;
        double years = time_series.size() - 1.0;
        return (std::pow(last.second / first.second, 1.0 / years) - 1.0) * 100.0;
    }

public:
    FactorScreener(const Config& cfg)
        : config(cfg),
          db_processor() {
        load_ticker_to_cik();
    }

    InvestableUniverse build() {
        InvestableUniverse universe;
        std::vector<Stock> new_stocks;

        auto cik_results = db_processor.execute_custom_query("SELECT DISTINCT cik FROM sub;");
        std::vector<int> ciks;
        for (const auto& row : cik_results) {
            ciks.push_back(std::stoi(row.at("cik")));
        }

        for (int cik : ciks) {
            auto it = cik_to_ticker.find(cik);
            if (it == cik_to_ticker.end()) {
                continue;
            }
            std::string ticker = it->second;

            std::cout << "Processing: " << ticker << std::endl;

            std::vector<std::string> tags = {"Revenues", "NetIncomeLoss", "StockholdersEquity", "Liabilities", "OperatingIncomeLoss", "CashAndCashEquivalentsAtCarryingValue", "CostOfRevenue", "CapitalExpenditures"};
            auto db_metrics = db_processor.query_all_latest_fundamentals(tags, 0, "USD", "");

            if (db_metrics.find(cik) == db_metrics.end()) {
                continue;
            }
            auto& cik_metrics = db_metrics[cik];

            bool valid = true;
            for (const auto& tag : {"Revenues", "NetIncomeLoss", "StockholdersEquity", "Liabilities", "OperatingIncomeLoss", "CashAndCashEquivalentsAtCarryingValue"}) {
                if (cik_metrics.find(tag) == cik_metrics.end()) {
                    std::cerr << "Skipping " << ticker << ": Missing " << tag << std::endl;
                    valid = false;
                    break;
                }
            }
            if (!valid) continue;

            double revenue = cik_metrics["Revenues"].second;
            double net_income = cik_metrics["NetIncomeLoss"].second;
            double equity = cik_metrics["StockholdersEquity"].second;
            double liabilities = cik_metrics["Liabilities"].second;
            double op_income = cik_metrics["OperatingIncomeLoss"].second;
            double cash = cik_metrics["CashAndCashEquivalentsAtCarryingValue"].second;
            double cost_revenue = cik_metrics.find("CostOfRevenue") != cik_metrics.end() ? cik_metrics["CostOfRevenue"].second : 0.0;
            double capex = cik_metrics.find("CapitalExpenditures") != cik_metrics.end() ? cik_metrics["CapitalExpenditures"].second : 0.0;

            if (equity <= 0.0 || revenue <= 0.0 || (equity + liabilities - cash) <= 0.0 || net_income < 0.0 || op_income < 0.0) {
                std::cerr << "Skipping " << ticker << ": Invalid values (zero/negative)" << std::endl;
                continue;
            }

            auto revenue_series = db_processor.query_time_series(cik, "Revenues", 4, "USD", "20190101", "20231231");
            double revenue_growth = calculate_cagr(revenue_series);
            if (revenue_series.empty()) {
                std::cerr << "Skipping " << ticker << ": No revenue time series" << std::endl;
                continue;
            }

            std::map<std::string, double> fund_metrics;
            fund_metrics["revenue_growth"] = revenue_growth;
            fund_metrics["roe"] = (net_income / equity) * 100.0;
            fund_metrics["debt_equity"] = liabilities / equity;
            fund_metrics["profit_margin"] = (net_income / revenue) * 100.0;
            fund_metrics["roic"] = (op_income * (1 - 0.21)) / (equity + liabilities - cash) * 100.0;
            fund_metrics["gross_margin"] = ((revenue - cost_revenue) / revenue) * 100.0;
            fund_metrics["fcf_approx"] = (op_income - capex) / revenue * 100.0;

            if (fund_metrics["roe"] <= 15.0 ||
                fund_metrics["debt_equity"] >= 0.5 ||
                fund_metrics["profit_margin"] <= 10.0 ||
                fund_metrics["revenue_growth"] <= 10.0 ||
                fund_metrics["roic"] <= 20.0 ||
                fund_metrics["gross_margin"] <= 40.0 ||
                fund_metrics["fcf_approx"] <= 5.0) {
                continue;
            }

            Stock s;
            s.ticker = ticker;
            new_stocks.push_back(s);
        }

        for (const auto& stock : new_stocks) {
            std::cout << "Investable Ticker: " << stock.ticker << std::endl;
        }

        universe.set_stocks(new_stocks);
        universe.save_current(config.current_universe_path);
        return universe;
    }
};

#endif
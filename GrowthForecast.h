#ifndef GROWTH_FORECAST_H
#define GROWTH_FORECAST_H

#include "Stock.h"
#include "Config.h"
#include <vector>
#include <random>
#include <cmath>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

class GrowthForecast {
private:
    const Config& config;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    json fetch_from_api(const std::string& endpoint) {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            std::string url = "https://api.polygon.io" + endpoint + "&apiKey=" + config.api_key;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                return json();
            }
        }
        try {
            return json::parse(readBuffer);
        } catch (...) {
            return json();
        }
    }

    double bottom_up_revenue(const Stock& stock) const {
        double market_size = 166.9e9;
        double penetration = stock.market_cap / market_size;
        double pricing = stock.pe_ratio * stock.revenue_growth;
        double timeline_adjust = 0.8;
        double base_revenue = market_size * penetration * pricing * timeline_adjust;
        double prob_adjust = 0.7;
        return base_revenue * prob_adjust;
    }

    double cost_margin_model(const Stock& stock, double revenue) const {
        double op_ex = revenue * (1 - stock.profit_margin);
        double r_and_d = revenue * 0.05;
        double sg_and_a = revenue * 0.2;
        double cogs = revenue * (1 - stock.fcf_yield);
        double margins_expansion = stock.profit_margin + 0.015;
        double capex = revenue * 0.1;
        return op_ex + r_and_d + sg_and_a + cogs + capex - (revenue * margins_expansion);
    }

    double dcf_core(const Stock& stock, double revenue, double costs) const {
        double ebit = revenue - costs;
        double fcf = ebit * (1 - config.dcf_tax_rate);
        double fcf_sum = 0.0;
        for (int t = 1; t <= config.dcf_forecast_horizon; ++t) {
            fcf_sum += fcf / std::pow(1 + config.dcf_wacc, t);
        }
        double tv = (fcf * (1 + config.dcf_terminal_growth_rate)) / (config.dcf_wacc - config.dcf_terminal_growth_rate);
        tv /= std::pow(1 + config.dcf_wacc, config.dcf_forecast_horizon);
        return fcf_sum + tv;
    }

    double risk_sensitivity_adjust(const Stock& stock, double base_dcf) const {
        std::default_random_engine generator;
        std::normal_distribution<double> dist(0.0, 0.1);
        double sum_adjust = 0.0;
        double best_prob = 0.3;
        double base_prob = 0.5;
        double worst_prob = 0.2;
        for (int scen = 0; scen < config.monte_carlo_simulations; ++scen) {
            double adjust = dist(generator);
            double scen_dcf = base_dcf * (1 + adjust);
            sum_adjust += best_prob * (scen_dcf * 1.2) + base_prob * scen_dcf + worst_prob * (scen_dcf * 0.8);
        }
        double damped_g = stock.revenue_growth / std::log10(stock.market_cap + 1);
        return (sum_adjust / config.monte_carlo_simulations) * damped_g;
    }

    double reverse_dcf_implied_g(const Stock& stock, double price, double base_fcf) const {
        double fcf_sum = 0.0;
        for (int t = 1; t <= config.dcf_forecast_horizon; ++t) {
            base_fcf *= (1 + stock.revenue_growth);
            fcf_sum += base_fcf / std::pow(1 + config.dcf_wacc, t);
        }
        double tv = price - fcf_sum;
        tv *= std::pow(1 + config.dcf_wacc, config.dcf_forecast_horizon);
        double implied_g = config.dcf_wacc - (base_fcf * (1 + config.dcf_terminal_growth_rate) / tv);
        return implied_g;
    }

    std::pair<double, double> monte_carlo_reverse_dcf(const Stock& stock, double price) const {
        double base_fcf = stock.market_cap * stock.fcf_yield;
        std::default_random_engine generator;
        std::normal_distribution<double> dist(0.0, config.monte_carlo_volatility_assumption);
        double sum_g = 0.0;
        double sum_sq = 0.0;
        for (int scen = 0; scen < config.monte_carlo_simulations; ++scen) {
            double scen_fcf = base_fcf * (1 + dist(generator));
            double implied_g = reverse_dcf_implied_g(stock, price, scen_fcf);
            sum_g += implied_g;
            sum_sq += implied_g * implied_g;
        }
        double mean_g = sum_g / config.monte_carlo_simulations;
        double var_g = (sum_sq / config.monte_carlo_simulations) - (mean_g * mean_g);
        double std_g = std::sqrt(std::abs(var_g));
        return {mean_g, std_g};
    }

public:
    GrowthForecast(const Config& cfg) : config(cfg) {}

    double forecast(const Stock& stock, double current_price) const {
        double revenue = bottom_up_revenue(stock);
        double costs = cost_margin_model(stock, revenue);
        double base_dcf = dcf_core(stock, revenue, costs);
        double adjusted_dcf = risk_sensitivity_adjust(stock, base_dcf);
        auto [mean_g, std_g] = monte_carlo_reverse_dcf(stock, current_price);
        double damped_mean_g = mean_g / std::log10(stock.market_cap + 1);
        return damped_mean_g - 0.2 * std_g;
    }
};

#endif
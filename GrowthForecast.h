#ifndef GROWTH_FORECAST_H
#define GROWTH_FORECAST_H

#include "Stock.h"
#include <vector>
#include <random>
#include <cmath>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

/* DCF Forecasting with Reverse DCF and Monte Carlo
This class implements a bottom-up discounted cash flow (DCF) forecasting model for stock valuation.
combined with reverse DCF to infer implied growth and Monte Carlo simulation for risk-adjusted variance.
It projects free cash flows over a horizon, adds a terminal value, and adjusts for probabilities and scale damping.

Bottom-Up Revenue Projection
R_t = sum_p (MS_p * PR_p * PN_p) * Prob_p

MS_p: Market size for product p (e.g., hardcoded or fetched sector TAM). Explanation: Represents the total addressable market for the company's core offerings.
PR_p: Pricing per unit (derived from PE ratio * revenue growth). Explanation: Captures the expected price point, adjusted for growth expectations.
PN_p: Penetration or units sold (market cap as proxy for share). Explanation: Estimates market penetration, factoring in timeline adjustments for launch delays.
Prob_p: Probability adjustment (e.g., 0.7 for tech launch success). Explanation: Incorporates risk of failure or delay, based on industry averages.
Cost and Margin Modeling
C_t = R_t * (COGS% + OPEX% + RD% + SGA% + CAPEX%)

COGS%, OPEX%, etc.: Percentages of revenue, trending toward efficiency (e.g., margins expand 1.5% annually). Explanation: Models operating costs, R&D, selling/general/admin, goods sold, and capital expenditures as revenue fractions.
DCF Core
IV = sum_{t=1 to T} FCF_t / (1 + WACC)^t + TV / (1 + WACC)^T

FCF_t = EBIT_t * (1 - τ) (simplified, assuming dep/capex/WC net to zero for growth focus). Explanation: Free cash flow in year t, from EBIT after tax.
TV = FCF_{T+1} / (WACC - g_terminal) Explanation: Terminal value assuming perpetual growth post-horizon.
WACC: Weighted average cost of capital (e.g., 0.1). Explanation: Discount rate reflecting equity/debt costs.
Risk and Sensitivity Adjustments
IV_adj = sum_scen Prob_scen * IV_scen, with scen variations ~ Normal(0, 0.1)

Prob_scen: Weights for best/base/worst (0.3/0.5/0.2). Explanation: Scenario analysis for upside/downside, damped by scale: g = g_base / log10(market_cap + 1).
Reverse DCF Implied g
g = WACC - FCF_{T+1} / [(P - sum FCF_t / (1+WACC)^t) * (1+WACC)^T]

P: Current price (fetched). Explanation: Solves for growth implied by market price.
Monte Carlo for Variance
Sample 1000 paths around inputs (e.g., FCF ~ Normal(base, 0.1)), compute g per path, return mean/std.

Interpretation in Lifecycle Context Forecasts high g in disruption (low cap, undamped), moderates in scaling (moats sustain), flattens in maturity (damping dominates). Monte Carlo adds uncertainty for risk penalty in U_s. */

class GrowthForecast {
private:
    std::string api_key;
    double wacc = 0.1;
    double tax_rate = 0.25;
    int T = 10;
    int num_scenarios = 1000;
    double g_terminal_base = 0.03;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    json fetch_from_api(const std::string& endpoint) {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            std::string url = "https://api.polygon.io" + endpoint + "&apiKey=" + api_key;
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
        double fcf = ebit * (1 - tax_rate);
        double fcf_sum = 0.0;
        for (int t = 1; t <= T; ++t) {
            fcf_sum += fcf / std::pow(1 + wacc, t);
        }
        double tv = (fcf * (1 + g_terminal_base)) / (wacc - g_terminal_base);
        tv /= std::pow(1 + wacc, T);
        return fcf_sum + tv;
    }

    double risk_sensitivity_adjust(const Stock& stock, double base_dcf) const {
        std::default_random_engine generator;
        std::normal_distribution<double> dist(0.0, 0.1);
        double sum_adjust = 0.0;
        double best_prob = 0.3;
        double base_prob = 0.5;
        double worst_prob = 0.2;
        for (int scen = 0; scen < num_scenarios; ++scen) {
            double adjust = dist(generator);
            double scen_dcf = base_dcf * (1 + adjust);
            sum_adjust += best_prob * (scen_dcf * 1.2) + base_prob * scen_dcf + worst_prob * (scen_dcf * 0.8);
        }
        double damped_g = stock.revenue_growth / std::log10(stock.market_cap + 1);
        return (sum_adjust / num_scenarios) * damped_g;
    }

    double reverse_dcf_implied_g(const Stock& stock, double price, double base_fcf) const {
        double fcf_sum = 0.0;
        for (int t = 1; t <= T; ++t) {
            base_fcf *= (1 + stock.revenue_growth);
            fcf_sum += base_fcf / std::pow(1 + wacc, t);
        }
        double tv = price - fcf_sum;
        tv *= std::pow(1 + wacc, T);
        double implied_g = wacc - (base_fcf * (1 + g_terminal_base) / tv);
        return implied_g;
    }

    std::pair<double, double> monte_carlo_reverse_dcf(const Stock& stock, double price) const {
        double base_fcf = stock.market_cap * stock.fcf_yield;
        std::default_random_engine generator;
        std::normal_distribution<double> dist(0.0, 0.1);
        double sum_g = 0.0;
        double sum_sq = 0.0;
        for (int scen = 0; scen < num_scenarios; ++scen) {
            double scen_fcf = base_fcf * (1 + dist(generator));
            double implied_g = reverse_dcf_implied_g(stock, price, scen_fcf);
            sum_g += implied_g;
            sum_sq += implied_g * implied_g;
        }
        double mean_g = sum_g / num_scenarios;
        double var_g = (sum_sq / num_scenarios) - (mean_g * mean_g);
        double std_g = std::sqrt(var_g);
        return {mean_g, std_g};
    }

public:
    GrowthForecast(const std::string& key) : api_key(key) {}

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
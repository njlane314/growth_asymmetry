#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

struct Config {

    // --- API & General Settings ---
    std::string api_key = "YOUR_API_KEY_HERE"; 

    // --- File Paths ---
    std::string prior_universe_path = "prior_universe.csv";
    std::string current_universe_path = "current_universe.csv";
    std::string log_file_path = "framework_log.txt";

    // --- Universe Builder Settings ---
    int top_n_stocks = 50;                              // The number of top-scoring stocks to include in the final universe

    // --- Fundamental Analysis Settings ---
    int fundamentals_lookback_years = 2;                // Number of years to look back for calculating growth metrics

    // --- Sentiment Analysis Settings ---
    int sentiment_lookback_days = 90;                   // Number of days of historical price data to use for sentiment calculations
    int rsi_period = 14;                                // The period for the Relative Strength Index (RSI) calculation
    double default_beta = 1.2;                          // Default beta to use if it cannot be calculated

    // --- Growth Forecast Settings ---
    double dcf_wacc = 0.095;                            // Weighted Average Cost of Capital (discount rate) for the DCF model
    double dcf_tax_rate = 0.21;                         // Assumed corporate tax rate
    int dcf_forecast_horizon = 10;                      // Number of years to explicitly forecast free cash flows
    double dcf_terminal_growth_rate = 0.025;            // Assumed perpetual growth rate for the terminal value calculation
    int monte_carlo_simulations = 10000;                // Number of simulations to run for the Monte Carlo analysis
    double monte_carlo_volatility_assumption = 0.1;     // Standard deviation assumption for inputs in Monte Carlo simulation

    // --- Portfolio Allocator Settings ---
    double risk_aversion_gamma = 3.0;                   // Controls the trade-off between return and risk in the objective function
    double max_position_weight = 0.10;                  // Maximum weight any single stock can have in the portfolio (10%)
    double target_portfolio_volatility = 0.18;          // An optional cap on the total portfolio volatility (18% annualized)
    bool perform_major_rebalance = true;                // Flag to determine if a full re-optimization should occur
};

#endif // CONFIG_H
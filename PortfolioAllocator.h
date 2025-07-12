#ifndef PORTFOLIO_ALLOCATOR_H
#define PORTFOLIO_ALLOCATOR_H

#include "Universe.h"
#include "Config.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

class PortfolioAllocator {
private:
    const Config& config;

    std::vector<double> solve_mean_variance_qp(const std::vector<double>& mu, const std::vector<std::vector<double>>& sigma) const {
        std::cout << "--- NOTE: Using placeholder optimizer. Integrate a real QP solver for production. ---" << std::endl;
        int n = mu.size();
        if (n == 0) return {};
        return std::vector<double>(n, 1.0 / n);
    }

    std::vector<std::vector<double>> calculate_covariance_matrix(const std::vector<Stock>& stocks) const {
        int n = stocks.size();
        std::cout << "--- NOTE: Using placeholder covariance matrix. Integrate real historical data. ---" << std::endl;
        std::vector<std::vector<double>> sigma(n, std::vector<double>(n, 0.05));
        for (int i = 0; i < n; ++i) sigma[i][i] = 0.15;
        return sigma;
    }

public:
    PortfolioAllocator(const Config& cfg) : config(cfg) {}

    std::vector<double> allocate(const Universe& universe, const std::vector<double>& prior_weights) const {
        const auto& stocks = universe.get_stocks();
        int n = stocks.size();
        if (n == 0) return {};

        std::vector<double> mu(n);
        for (int i = 0; i < n; ++i) {
            mu[i] = stocks[i].forecasted_growth;
        }

        if (config.perform_major_rebalance) {
            auto sigma = calculate_covariance_matrix(stocks);
            return solve_mean_variance_qp(mu, sigma);
        } else {
            if (prior_weights.size() != n) {
                return std::vector<double>(n, 1.0 / n);
            }
            std::vector<double> w = prior_weights;
            double sum_delta = 0.0;
            for (int i = 0; i < n; ++i) {
                double delta = stocks[i].score * 0.05;
                w[i] += delta;
                sum_delta += delta;
            }
            for (int i = 0; i < n; ++i) w[i] -= sum_delta / n;
            double sum_w = 0.0;
            for (double wi : w) sum_w += wi;
            for (int i = 0; i < n; ++i) w[i] /= sum_w;
            for (int i = 0; i < n; ++i) {
                if (w[i] < 0.0) w[i] = 0.0;
                if (w[i] > config.max_position_weight) w[i] = config.max_position_weight;
            }
            sum_w = 0.0;
            for (double wi : w) sum_w += wi;
            for (int i = 0; i < n; ++i) w[i] /= sum_w;
            return w;
        }
    }
};

#endif
#ifndef PORTFOLIO_ALLOCATOR_H
#define PORTFOLIO_ALLOCATOR_H

#include "Universe.h"
#include <vector>
#include <algorithm>
#include <cmath>

/* The Mean-Variance Optimization Formula (Markowitz Model, Adapted for Growth Bias)
The objective is to maximize the expected portfolio return while penalizing risk,
with a bias toward growth stocks by incorporating forecasted growth rates into expected returns.

Objective Function
max_w ( w transpose mu - (gamma / 2) w transpose covariance matrix w )

w: The vector of portfolio weights (w_s for each stock s, summing to 1). Explanation: Represents the proportion of capital allocated to each stock in the universe.
mu: The vector of expected returns for each stock. Explanation: Derived from forecasted growth g_s (for example, mu_s = g_s or E[V_s] from DCF), capturing upside potential. For growth stocks, this biases toward high g_s while damping for scale.
covariance matrix: The covariance matrix of stock returns. Explanation: Measures how returns co-vary, penalizing correlated risks. Diagonal elements are variances (vol_s squared), off-diagonals covariances.
gamma: Risk aversion parameter (for example, 2 to 5). Explanation: Controls the trade-off; lower gamma favors aggressive growth (less risk penalty), higher gamma prioritizes stability.
Constraints
These ensure feasibility and risk control:

w transpose 1 = 1
Full allocation constraint.
Explanation: All capital must be invested (sum of weights = 1).
w >= 0
No short positions.
Explanation: Prevents betting against stocks, focusing on long-only growth.
w_s <= max_position (for example, 0.1)
Maximum position size per stock.
Explanation: Limits concentration risk, promoting diversification in volatile growth stocks.
sum w_s * vol_s <= target_vol (for example, 0.15)
Portfolio volatility cap.
Explanation: Constrains total risk (weighted average volatility), balancing growth upside with downside protection.
Interpretation in Lifecycle Context
This formula optimizes for asymmetric growth by favoring high mu (from undamped g in disruption phases)
but penalizing via covariance matrix and constraints in maturity (where vol rises with scale). Daily updates use prior w
for minor changes (adjust proportionally to delta score), full re-optimization only on major signals
(for example, universe delta >20%), minimizing trades. Aesthetic in its elegance: A quadratic program solving
the eternal balance of reward and risk.
*/

class PortfolioAllocator {
private:
    double gamma = 3.0;
    double max_position = 0.1;
    double target_vol = 0.15;

    double compute_return(const std::vector<double>& w, const std::vector<double>& mu) const {
        double ret = 0.0;
        for (size_t i = 0; i < w.size(); ++i) {
            ret += w[i] * mu[i];
        }
        return ret;
    }

    double compute_risk(const std::vector<double>& w, const std::vector<std::vector<double>>& sigma) const {
        double risk = 0.0;
        for (size_t i = 0; i < w.size(); ++i) {
            for (size_t j = 0; j < w.size(); ++j) {
                risk += w[i] * sigma[i][j] * w[j];
            }
        }
        return risk;
    }

    double compute_portfolio_vol(const std::vector<double>& w, const std::vector<std::vector<double>>& sigma) const {
        return std::sqrt(compute_risk(w, sigma));
    }

    bool check_constraints(const std::vector<double>& w, double port_vol) const {
        double sum_w = 0.0;
        for (double wi : w) {
            sum_w += wi;
            if (wi < 0.0 || wi > max_position) return false;
        }
        if (std::abs(sum_w - 1.0) > 1e-6 || port_vol > target_vol) return false;
        return true;
    }

public:
    PortfolioAllocator() {}

    std::vector<double> allocate(const Universe& universe, const std::vector<double>& prior_weights, bool major_change) const {
        const auto& stocks = universe.get_stocks();
        int n = stocks.size();
        if (n == 0) return {};
        std::vector<double> mu(n);
        for (int i = 0; i < n; ++i) {
            mu[i] = stocks[i].forecasted_growth;
        }
        std::vector<std::vector<double>> sigma(n, std::vector<double>(n, 0.05));
        for (int i = 0; i < n; ++i) sigma[i][i] = 0.15;
        std::vector<double> w = prior_weights.empty() ? std::vector<double>(n, 1.0 / n) : prior_weights;
        if (!major_change) {
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
            for (int i = 0; i < n; ++i) if (w[i] < 0.0) w[i] = 0.0;
            for (int i = 0; i < n; ++i) if (w[i] > max_position) w[i] = max_position;
            sum_w = 0.0;
            for (double wi : w) sum_w += wi;
            for (int i = 0; i < n; ++i) w[i] /= sum_w;
            return w;
        }
        double best_objective = -std::numeric_limits<double>::infinity();
        std::vector<double> best_w = w;
        for (int iter = 0; iter < 1000; ++iter) {
            double ret = compute_return(w, mu);
            double risk = compute_risk(w, sigma);
            double objective = ret - (gamma / 2.0) * risk;
            double port_vol = compute_portfolio_vol(w, sigma);
            if (objective > best_objective && check_constraints(w, port_vol)) {
                best_objective = objective;
                best_w = w;
            }
            int idx = rand() % n;
            w[idx] += (rand() % 100 - 50) / 1000.0;
            double sum_w = 0.0;
            for (double wi : w) sum_w += wi;
            for (int i = 0; i < n; ++i) w[i] /= sum_w;
            for (int i = 0; i < n; ++i) if (w[i] < 0.0) w[i] = 0.0;
            for (int i = 0; i < n; ++i) if (w[i] > max_position) w[i] = max_position;
            sum_w = 0.0;
            for (double wi : w) sum_w += wi;
            for (int i = 0; i < n; ++i) w[i] /= sum_w;
        }
        return best_w;
    }
};

#endif
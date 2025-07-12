#ifndef MSAR_H
#define MSAR_H

#include <vector>
#include <cmath>
#include <random>
#include <numeric>
#include <limits>

class MarketSentiment {
private:
    int K;
    int p;
    int n_vars;
    std::vector<std::vector<double>> trans_prob;
    std::vector<double> start_prob;
    std::vector<std::vector<double>> means;
    std::vector<std::vector<std::vector<double>>> covs;
    std::vector<std::vector<double>> ar_coeffs;

    double log_likelihood(const std::vector<std::vector<double>>& data, const std::vector<std::vector<double>>& gamma) const {
        double ll = 0.0;
        for (size_t t = 0; t < data.size(); ++t) {
            for (int k = 0; k < K; ++k) {
                ll += gamma[t][k] * std::log(normal_pdf(data[t], means[k], covs[k]));
            }
        }
        return ll;
    }

    double normal_pdf(const std::vector<double>& x, const std::vector<double>& mean, const std::vector<std::vector<double>>& cov) const {
        double det = 1.0;
        for (size_t i = 0; i < cov.size(); ++i) det *= cov[i][i];
        double exp_term = 0.0;
        for (size_t i = 0; i < x.size(); ++i) exp_term += (x[i] - mean[i]) * (x[i] - mean[i]) / cov[i][i];
        return std::exp(-0.5 * exp_term) / std::sqrt(std::pow(2 * M_PI, x.size()) * det);
    }

public:
    MarketSentiment(int regimes = 2, int order = 1, int vars = 1) : K(regimes), p(order), n_vars(vars) {
        trans_prob.assign(K, std::vector<double>(K, 1.0 / K));
        start_prob.assign(K, 1.0 / K);
        means.assign(K, std::vector<double>(n_vars, 0.0));
        covs.assign(K, std::vector<std::vector<double>>(n_vars, std::vector<double>(n_vars, 0.0)));
        for (int k = 0; k < K; ++k) for (int i = 0; i < n_vars; ++i) covs[k][i][i] = 1.0;
        ar_coeffs.assign(K, std::vector<double>(p * n_vars, 0.0));
    }

    void fit(const std::vector<std::vector<double>>& data, int max_iter = 50, double tol = 1e-6) {
        int T = data.size();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        for (int k = 0; k < K; ++k) for (int j = 0; j < K; ++j) trans_prob[k][j] = dis(gen);
        for (int k = 0; k < K; ++k) {
            double sum = 0.0;
            for (int j = 0; j < K; ++j) sum += trans_prob[k][j];
            for (int j = 0; j < K; ++j) trans_prob[k][j] /= sum;
        }
        for (int k = 0; k < K; ++k) for (int v = 0; v < n_vars; ++v) means[k][v] = dis(gen) * 0.1;

        double old_ll = -std::numeric_limits<double>::infinity();
        for (int iter = 0; iter < max_iter; ++iter) {
            std::vector<std::vector<double>> alpha(T, std::vector<double>(K, 0.0));
            std::vector<double> scale(T, 0.0);
            alpha[0] = start_prob;
            for (int k = 0; k < K; ++k) alpha[0][k] *= normal_pdf(data[0], means[k], covs[k]);
            scale[0] = std::accumulate(alpha[0].begin(), alpha[0].end(), 0.0);
            for (int k = 0; k < K; ++k) alpha[0][k] /= scale[0];
            for (int t = 1; t < T; ++t) {
                for (int k = 0; k < K; ++k) {
                    alpha[t][k] = 0.0;
                    for (int j = 0; j < K; ++j) alpha[t][k] += alpha[t-1][j] * trans_prob[j][k];
                    alpha[t][k] *= normal_pdf(data[t], means[k], covs[k]);
                }
                scale[t] = std::accumulate(alpha[t].begin(), alpha[t].end(), 0.0);
                for (int k = 0; k < K; ++k) alpha[t][k] /= scale[t];
            }

            std::vector<std::vector<double>> beta(T, std::vector<double>(K, 0.0));
            for (int k = 0; k < K; ++k) beta[T-1][k] = 1.0 / scale[T-1];
            for (int t = T-2; t >= 0; --t) {
                for (int k = 0; k < K; ++k) {
                    beta[t][k] = 0.0;
                    for (int j = 0; j < K; ++j) beta[t][k] += trans_prob[k][j] * normal_pdf(data[t+1], means[j], covs[j]) * beta[t+1][j];
                    beta[t][k] /= scale[t];
                }
            }

            std::vector<std::vector<double>> gamma(T, std::vector<double>(K, 0.0));
            for (int t = 0; t < T; ++t) for (int k = 0; k < K; ++k) gamma[t][k] = alpha[t][k] * beta[t][k];

            std::vector<std::vector<std::vector<double>>> xi(T-1, std::vector<std::vector<double>>(K, std::vector<double>(K, 0.0)));
            for (int t = 0; t < T-1; ++t) {
                double sum = 0.0;
                for (int i = 0; i < K; ++i) for (int j = 0; j < K; ++j) {
                    xi[t][i][j] = alpha[t][i] * trans_prob[i][j] * normal_pdf(data[t+1], means[j], covs[j]) * beta[t+1][j];
                    sum += xi[t][i][j];
                }
                for (int i = 0; i < K; ++i) for (int j = 0; j < K; ++j) xi[t][i][j] /= sum;
            }

            for (int k = 0; k < K; ++k) start_prob[k] = gamma[0][k];

            for (int i = 0; i < K; ++i) {
                double sum_i = 0.0;
                for (int t = 0; t < T-1; ++t) sum_i += gamma[t][i];
                for (int j = 0; j < K; ++j) {
                    double sum_ij = 0.0;
                    for (int t = 0; t < T-1; ++t) sum_ij += xi[t][i][j];
                    trans_prob[i][j] = sum_ij / sum_i;
                }
            }

            for (int k = 0; k < K; ++k) for (int v = 0; v < n_vars; ++v) {
                double sum_gamma = 0.0;
                double sum_mean = 0.0;
                for (int t = 0; t < T; ++t) {
                    sum_gamma += gamma[t][k];
                    sum_mean += gamma[t][k] * data[t][v];
                }
                means[k][v] = sum_mean / sum_gamma;
            }

            double ll = 0.0;
            for (int t = 0; t < T; ++t) ll += std::log(scale[t]);
            if (std::abs(ll - old_ll) < tol) break;
            old_ll = ll;
        }
    }

    std::vector<double> predict_regime(const std::vector<std::vector<double>>& data) const {
        int T = data.size();
        std::vector<std::vector<double>> alpha(T, std::vector<double>(K, 0.0));
        alpha[0] = start_prob;
        for (int k = 0; k < K; ++k) alpha[0][k] *= normal_pdf(data[0], means[k], covs[k]);
        double scale = std::accumulate(alpha[0].begin(), alpha[0].end(), 0.0);
        for (int k = 0; k < K; ++k) alpha[0][k] /= scale;
        for (int t = 1; t < T; ++t) {
            for (int k = 0; k < K; ++k) {
                alpha[t][k] = 0.0;
                for (int j = 0; j < K; ++j) alpha[t][k] += alpha[t-1][j] * trans_prob[j][k];
                alpha[t][k] *= normal_pdf(data[t], means[k], covs[k]);
            }
            scale = std::accumulate(alpha[t].begin(), alpha[t].end(), 0.0);
            for (int k = 0; k < K; ++k) alpha[t][k] /= scale;
        }
        return alpha.back();
    }
};

#endif
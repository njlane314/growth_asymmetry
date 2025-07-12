#ifndef ALPHA_DECAY_MODEL_H
#define ALPHA_DECAY_MODEL_H

#include "Universe.h"
#include "MarketSentiment.h"
#include <vector>
#include <numeric>
#include <algorithm>

class AlphaDecayModel {
public:
    AlphaDecayModel(double score_change_threshold = 0.15, int top_n_to_check = 10)
        : score_change_threshold(score_change_threshold),
          top_n_to_check(top_n_to_check),
          last_regime(-1) {}

    bool has_alpha_decayed(const Universe& new_universe, const Universe& old_universe, const MarketSentiment& sentiment) {
        auto predicted_regime_prob = sentiment.predict_regime({}); 
        int current_regime = std::distance(predicted_regime_prob.begin(), std::max_element(predicted_regime_prob.begin(), predicted_regime_prob.end()));
        if (last_regime != -1 && current_regime != last_regime) {
            last_regime = current_regime;
            return true;
        }
        last_regime = current_regime;

        double avg_new_score = calculate_average_score(new_universe.get_stocks());
        double avg_old_score = calculate_average_score(old_universe.get_stocks());

        if (avg_old_score > 0 && std::abs((avg_new_score - avg_old_score) / avg_old_score) > score_change_threshold) {
            return true;
        }

        return false;
    }

private:
    double score_change_threshold;
    int top_n_to_check;
    int last_regime;

    double calculate_average_score(const std::vector<Stock>& stocks) const {
        if (stocks.empty()) return 0.0;
        double sum = 0.0;
        int count = 0;
        for (const auto& stock : stocks) {
            if (count >= top_n_to_check) break;
            sum += stock.score;
            count++;
        }
        return (count > 0) ? (sum / count) : 0.0;
    }
};

#endif
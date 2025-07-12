# AsymmetricGrowthFramework

This repository implements a C++ framework for identifying stocks with asymmetric growth potential and optimising risk-adjusted portfolio allocation. Below is a detailed explanation of the mathematics, integrating screening, forecasting, sentiment, optimisation, and regime detection.

## Asymmetric Growth Investing

From the foundational principle that value is the discounted sum of future cash flows, \( V = \sum_{t=1}^{\infty} \frac{CF_t}{(1 + r)^t} \), where \( CF_t \) is cash flow at time \( t \) and \( r \) is the discount rate, this framework constructs a system to identify and allocate to stocks exhibiting asymmetric growth—outsized upside from innovation compounding, tempered by risk controls. It integrates screening for potential, forecasting for realism, sentiment for momentum, optimisation for efficiency, and regime detection for safety, drawing on philosophies of rigorous valuation, quality compounding, and sustainable growth. Markets are semi-efficient, reflecting public information rapidly but susceptible to behavioral biases, allowing edges in early detection of undervalued disruptors.

### Stock Screening: Utility for Asymmetry

Screening begins with the utility function, balancing growth, sentiment, and risk to rank stocks:

$$ U_s = \alpha \cdot G_s + \beta \cdot \Delta S_s - \lambda \cdot R_s $$

Here, \( G_s \) captures forecasted growth (detailed below), \( \Delta S_s \) is sentiment change, \( R_s \) penalizes volatility, and weights \( \alpha, \beta, \lambda \) (e.g., 0.5, 0.3, 0.2) emphasise upside. Threshold \( U_s > 0.7 \) selects candidates, prioritising those where \( G_s \) signals exponential compounding (\( g > 20\% \)) in disruption phases, damped for maturity via \( g_{adj} = g / \log_{10}(\text{market_cap} + 1) \).

This works because it quantifies asymmetry: High \( G_s \) identifies scalable innovation (e.g., AI chips), \( \Delta S_s > 0.05 \) confirms rising validation, and \( R_s \) (e.g., vol + |RSI - 50|/50) avoids hype bubbles, aligning with Buffett's moat focus (ROE >15%) for sustainable edges.

### Growth Forecasting: DCF and Reverse DCF

Forecasting employs discounted cash flow (DCF) to estimate intrinsic value:

$$ IV = \sum_{t=1}^{T} \frac{FCF_t}{(1 + WACC)^t} + \frac{FCF_{T+1} / (WACC - g)}{(1 + WACC)^T} $$

where \( FCF_t = R_t \cdot (1 - \tau) - \Delta WC_t - CapEx_t \), \( R_t \) is revenue from bottom-up projections (market_size \cdot penetration \cdot pricing \cdot Prob), and \( \tau \) is tax rate. Terminal growth \( g \) is damped for realism.

Reverse DCF inverts to implied \( g \):

$$ g = WACC - \frac{FCF_{T+1}}{(P - \sum_{t=1}^{T} \frac{FCF_t}{(1 + WACC)^t}) \cdot (1 + WACC)^T} $$

Monte Carlo samples inputs (e.g., FCF ~ Normal(base, 0.1)) for mean/std implied g, adjusting \( G_s = \text{mean_g} - 0.2 \cdot \text{std_g} \), damped by market cap.

Logically, this projects cash flows from fundamentals while reverse-checking market assumptions (if implied g > sustainable 15%, overvalued), enabling early detection of undervalued compounders.

### Sentiment Analysis: Quantitative Momentum

Sentiment score \( S_s \) proxies market mood:

$$ S_s = (50 - \text{RSI} \cdot 0.3) + (\text{vol} \cdot -0.2) + (\beta \cdot 0.1) + 0.6 $$

RSI = \( 100 - 100 / (1 + \text{Avg Gain} / \text{Avg Loss}) \), vol = \( \sqrt{\sum (r_t - \bar{r})^2 / N} \cdot \sqrt{252} \).

\( \Delta S_s \) tracks rising positivity. This works as a filter: Balanced RSI (30-70) signals sustainable momentum, low vol favors stability, integrating with U_s for asymmetry without noise.

### Portfolio Optimisation: Mean-Variance with Growth Bias

Weights maximize risk-adjusted return:

$$ \max_w \left( w^T \mu - \frac{\gamma}{2} w^T \Sigma w \right) $$

Subject to \( w^T 1 = 1 \), \( w \geq 0 \), \( w_s \leq 0.1 \), \( \sum w_s \cdot \sigma_s \leq 0.15 \), where \( \mu \) from forecasted g, \( \Sigma \) covariance.

Iterative perturbations find optimal w, minor changes adjust proportionally to \( \Delta U_s \). Logically, this allocates to high-mu growth while constraining risk, fitting lifecycle by favoring scaling-phase stocks (moderate sigma).

### Markov Hidden State: MSAR Regimes

To exit during meltdowns, MSAR detects regimes (bull/bear) from multivariate series (GDP, VIX, indexes):

$$ y_t = \mu_{s_t} + \sum_{i=1}^p \phi_{s_t,i} (y_{t-i} - \mu_{s_t}) + \epsilon_t, \quad \epsilon_t \sim N(0, \Sigma_{s_t}) $$

s_t follows Markov chain with trans_prob P(s_t = j | s_{t-1} = i). EM fits parameters: E-step computes gamma_t(k) = P(s_t = k | data) via forward-backward, M-step updates trans_prob, mu, Sigma.

Prediction: P(s_T = bear | data) from alpha_T(k); if >0.6, reduce exposure (e.g., scale w by 0.5).

Logically, MSAR captures latent shifts (high VIX/low GDP in bear), enabling proactive de-risking before crashes, complementing screening by preserving gains in downturns.

### Logical Flow in Detail

The framework operates as a closed loop: Daily, screen via U_s (fundamentals + sentiment), forecast g with DCF/reverse for G_s, optimize w if signals major (ΔU_s >0.1 or regime shift). Logically, screening isolates asymmetry (high G_s, low R_s), forecasting validates realism (implied vs. sustainable g), sentiment adds timing (rising ΔS_s for momentum), optimisation allocates efficiently (max return/min risk), and MSAR safeguards (bear regime reduces w). This creates a resilient system: Early detection of disruptors, compounded allocation, and meltdown exits, rooted in value principles for long-term outperformance.
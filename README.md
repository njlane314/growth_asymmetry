## Utility Function for Stock Selection

Stocks are evaluated using a utility function that balances growth, sentiment, and risk:

$$U_s = \alpha \cdot G_s + \beta \cdot \Delta S_s - \lambda \cdot R_s$$

-   $G_s$: Forecasted growth potential, derived from models like DCF.
-   $\Delta S_s$: Change in sentiment score over time.
-   $R_s$: Risk measure, capturing volatility and downside.
-   $\alpha, \beta, \lambda$: Weights adjusting the emphasis on each factor.

## Portfolio Optimisation

Portfolio weights are optimized using a mean-variance approach tailored for growth:

$$\max_w \left( w^T \mu - \frac{\gamma}{2} w^T \Sigma w \right)$$

Subject to:

$$w^T 1 = 1, \quad w \geq 0, \quad w_s \leq w_{\text{max}}, \quad \sum w_s \cdot \sigma_s \leq \sigma_{\text{target}}$$

-   $w$: Vector of portfolio weights.
-   $\mu$: Expected returns vector.
-   $\Sigma$: Covariance matrix of returns.
-   $\gamma$: Risk aversion parameter.
-   $w_{\text{max}}$: Maximum weight per stock.
-   $\sigma_{\text{target}}$: Target portfolio volatility.

## Growth Forecasting

### Discounted Cash Flow (DCF)

Intrinsic value is calculated as:

$$IV = \sum_{t=1}^{T} \frac{FCF_t}{(1 + WACC)^t} + \frac{TV}{(1 + WACC)^T}$$

Where free cash flow is:

$$FCF_t = R_t \cdot (1 - \text{taxrate}) - \Delta WC_t - CapEx_t$$

And terminal value is:

$$TV = \frac{FCF_{T+1}}{WACC - g}$$

-   $R_t$: Revenue at time $t$.
-   $\Delta WC_t$: Change in working capital.
-   $CapEx_t$: Capital expenditures.
-   $WACC$: Weighted average cost of capital.
-   $g$: Perpetual growth rate.

## Sentiment Analysis

Sentiment is quantified with a composite score:

$$S_s = w_1 \cdot (50 - \text{RSI} \cdot 0.3) + w_2 \cdot (\text{vol} \cdot -0.2) + w_3 \cdot (\beta \cdot 0.1) + c$$

### Relative Strength Index (RSI)

$$\text{RSI} = 100 - \frac{100}{1 + \frac{\text{Avg Gain}}{\text{Avg Loss}}}$$

### Volatility

$$\text{vol} = \sqrt{\frac{1}{N} \sum (r_t - \bar{r})^2} \cdot \sqrt{252}$$

-   $r_t$: Daily returns.
-   $\bar{r}$: Mean return.
-   $N$: Number of observations.
-   $\beta$: Stock’s market sensitivity.
-   $w_1, w_2, w_3, c$: Calibration constants.
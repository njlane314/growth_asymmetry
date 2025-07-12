#ifndef TRADE_H
#define TRADE_H

#include <string>

enum class TradeType {
    BUY,
    SELL
};

struct Trade {
    std::string ticker;
    TradeType type;
    int quantity;
    double price;
};

#endif
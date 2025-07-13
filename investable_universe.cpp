#include "framework/FinancialProcessor.h"
#include "framework/FactorScreener.h"
#include "framework/Config.h"

#include <iostream>

int main() {
    FinancialProcessor processor;

    processor.process_quarter("2024q3");
    processor.process_quarter("2024q4");
    processor.process_quarter("2025q1");
    processor.process_quarter("2025q2");

    processor.print_db_summary();

    Config cfg;
    FactorScreener screener(cfg);
    InvestableUniverse investable_universe = screener.build();

    return 0;
}
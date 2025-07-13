#include "framework/FinancialProcessor.h"
#include <iostream>

int main() {
    FinancialProcessor processor;

    //std::cout << "Attempting to process a single quarter: 2024q1" << std::endl;
    processor.process_quarter("2024q1");

    /*auto latest_net_income = processor.query_fundamentals(320193, "NetIncomeLoss");
    for (const auto& entry : latest_net_income) {
        std::cout << "Date: " << entry.first << ", Value: " << entry.second << std::endl;
    }

    auto revenue_series = processor.query_time_series(789019, "Revenues", 1, "USD", "20200101", "20250630");
    for (const auto& entry : revenue_series) {
        std::cout << "Date: " << entry.first << ", Revenue: " << entry.second << std::endl;
    }*/

    std::string sql = R"(
        SELECT s.cik, s.name, n.ddate AS end_date, n.value AS assets, t.tlabel AS description
        FROM num n
        JOIN sub s ON n.adsh = s.adsh
        JOIN tag t ON n.tag = t.tag AND n.version = t.version
        WHERE s.cik = 320193
          AND n.tag = 'Assets'
          AND n.qtrs = 0
          AND n.uom = 'USD'
          AND n.segments = ''
        ORDER BY n.ddate DESC
        LIMIT 1;
    )";

    auto results = processor.execute_custom_query(sql);
    if (results.empty()) {
        std::cout << "No results found. Ensure the quarter is processed and data exists." << std::endl;
    } else {
        for (const auto& row : results) {
            std::cout << "CIK: " << row.at("cik")
                      << ", Name: " << row.at("name")
                      << ", End Date: " << row.at("end_date")
                      << ", Assets: " << row.at("assets")
                      << ", Description: " << row.at("description") << std::endl;
        }
    }

    std::vector<std::string> tags = {"Assets", "Liabilities", "Revenues", "NetIncomeLoss", "StockholdersEquity", "AssetsCurrent", "LiabilitiesCurrent", "EarningsPerShareBasic", "CashAndCashEquivalentsAtCarryingValue", "OperatingIncomeLoss"};
    auto universe = processor.query_all_latest_fundamentals(tags, 0, "USD", "");  

    for (const auto& [cik, fundamentals] : universe) {
        if (fundamentals.count("Assets") && fundamentals.at("Assets").second > 1e9) {
            std::cout << "CIK: " << cik << ", Latest Assets: " << fundamentals.at("Assets").second << std::endl;
        }
    }

    processor.print_db_summary();

    // std::cout << "Attempting to process another single quarter: 2024q4" << std::endl;
    // processor.process_quarter("2024q4");

    /*FinancialProcessor processor;
    processor.process_all_quarters(); 
    auto results = processor.query_fundamentals("AAPL", "Revenues");
    for (const auto& pair : results) {
        std::cout << "Date: " << pair.first << ", Value: " << pair.second << std::endl;
    }*/
    return 0;
}
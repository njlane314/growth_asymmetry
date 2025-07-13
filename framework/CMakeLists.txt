add_library(framework INTERFACE)

target_link_libraries(framework INTERFACE nlohmann_json::nlohmann_json)

target_sources(framework INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Stock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InvestableUniverse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PositionBook.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FundamentalsAnalyser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SentimentAnalyser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GrowthForecast.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactorScreener.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MarketFeedProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PolygonFeedProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FinancialProcessor.h
)

find_package(CURL REQUIRED)
target_link_libraries(framework INTERFACE CURL::libcurl)
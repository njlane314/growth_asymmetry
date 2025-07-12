add_library(framework INTERFACE)

target_include_directories(framework INTERFACE
    /usr/local/lib/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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
)

find_package(CURL REQUIRED)
target_link_libraries(framework INTERFACE CURL::libcurl)
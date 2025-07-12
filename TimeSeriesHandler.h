#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include "Config.h"
#include "Stock.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

class TimeSeriesHandler {
public:
    TimeSeriesHandler(const Config& cfg) : config(cfg) {}

    void load_all_data() {
        std::cout << "Loading all historical data..." << std::endl;
        auto now = std::chrono::system_clock::now();
        auto to_date = now;
        auto from_date = now - std::chrono::hours(24 * 365 * 2); // 2 years of data
        std::time_t to_time = std::chrono::system_clock::to_time_t(to_date);
        std::time_t from_time = std::chrono::system_clock::to_time_t(from_date);
        std::stringstream to_ss, from_ss;
        to_ss << std::put_time(std::localtime(&to_time), "%Y-%m-%d");
        from_ss << std::put_time(std::localtime(&from_time), "%Y-%m-%d");
        
        for(const auto& ticker : config.initial_candidates) {
            if(!load_from_cache(ticker)){
                fetch_and_cache_data(ticker, from_ss.str(), to_ss.str());
            }
        }
    }

    const std::vector<std::string>& get_all_dates() const {
        // This should be dynamically populated from the loaded data
        // For now, returning a placeholder.
        static std::vector<std::string> dates;
        if(dates.empty() && !historical_data.empty()){
            for(const auto& pair : historical_data.begin()->second){
                dates.push_back(pair.first);
            }
        }
        return dates;
    }

    void set_current_date(const std::string& date) {
        current_date = date;
    }

    double get_price(const std::string& ticker) const {
        if (historical_data.count(ticker) && historical_data.at(ticker).count(current_date)) {
            return historical_data.at(ticker).at(current_date).at("close");
        }
        return 0.0;
    }

private:
    const Config& config;
    std::string current_date;
    std::map<std::string, std::map<std::string, std::map<std::string, double>>> historical_data;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    json fetch_from_api(const std::string& endpoint) {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if (curl) {
            std::string url = "https://api.polygon.io" + endpoint + "&apiKey=" + config.api_key;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
            }
        }
        try {
            return json::parse(readBuffer);
        } catch (const json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        }
    }

    void fetch_and_cache_data(const std::string& ticker, const std::string& from, const std::string& to) {
        std::cout << "Fetching data for " << ticker << " from " << from << " to " << to << std::endl;
        std::string endpoint = "/v2/aggs/ticker/" + ticker + "/range/1/day/" + from + "/" + to;
        try {
            json data = fetch_from_api(endpoint);
            save_to_cache(ticker, data);
            load_from_cache(ticker); // Load the newly cached data
        } catch (const std::runtime_error& e) {
            std::cerr << "Error fetching data for " << ticker << ": " << e.what() << std::endl;
        }
    }

    void save_to_cache(const std::string& ticker, const json& data) {
        std::ofstream file(ticker + ".csv");
        if (!file.is_open() || data.find("results") == data.end()) return;
        
        file << "date,open,high,low,close,volume\n";
        for (const auto& day : data["results"]) {
            long long ts = day["t"];
            std::time_t time = ts / 1000;
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&time), "%Y-%m-%d");
            file << ss.str() << ","
                 << day.value("o", 0.0) << ","
                 << day.value("h", 0.0) << ","
                 << day.value("l", 0.0) << ","
                 << day.value("c", 0.0) << ","
                 << day.value("v", 0.0) << "\n";
        }
    }

    bool load_from_cache(const std::string& ticker) {
        std::ifstream file(ticker + ".csv");
        if (!file.is_open()) return false;

        std::cout << "Loading data for " << ticker << " from cache." << std::endl;
        std::string line;
        std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string date, open, high, low, close, volume;
            std::getline(ss, date, ',');
            std::getline(ss, open, ',');
            std::getline(ss, high, ',');
            std::getline(ss, low, ',');
            std::getline(ss, close, ',');
            std::getline(ss, volume, ',');
            
            historical_data[ticker][date]["open"] = std::stod(open);
            historical_data[ticker][date]["high"] = std::stod(high);
            historical_data[ticker][date]["low"] = std::stod(low);
            historical_data[ticker][date]["close"] = std::stod(close);
            historical_data[ticker][date]["volume"] = std::stod(volume);
        }
        return true;
    }
};

#endif
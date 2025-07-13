#ifndef SEC_PROCESSOR_H
#define SEC_PROCESSOR_H

#include <sqlite3.h>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <curl/curl.h>
#include <zlib.h>
#include <minizip/unzip.h>
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <chrono>

class FinancialProcessor {
private:
    sqlite3* db;
    std::string db_file = "sec_data.db";
    std::string base_dir = "sec_data/";

    std::map<std::string, std::map<std::string, std::string>> table_column_types = {
        {"sub", {
            {"adsh", "TEXT(20)"},
            {"cik", "INTEGER(10)"},
            {"name", "TEXT(150)"},
            {"sic", "SMALLINT(4)"},
            {"countryba", "TEXT(2)"},
            {"stprba", "TEXT(2)"},
            {"cityba", "TEXT(30)"},
            {"zipba", "TEXT(10)"},
            {"bas1", "TEXT(40)"},
            {"bas2", "TEXT(40)"},
            {"baph", "TEXT(20)"},
            {"countryma", "TEXT(2)"},
            {"stprma", "TEXT(2)"},
            {"cityma", "TEXT(30)"},
            {"zipma", "TEXT(10)"},
            {"mas1", "TEXT(40)"},
            {"mas2", "TEXT(40)"},
            {"countryinc", "TEXT(3)"},
            {"stprinc", "TEXT(2)"},
            {"ein", "TEXT(10)"},
            {"former", "TEXT(150)"},
            {"changed", "TEXT(8)"},
            {"afs", "TEXT(5)"},
            {"wksi", "INTEGER"},
            {"fye", "TEXT(4)"},
            {"form", "TEXT(10)"},
            {"period", "TEXT(8)"},
            {"fy", "TEXT(4)"},
            {"fp", "TEXT(2)"},
            {"filed", "TEXT(8)"},
            {"accepted", "TEXT(19)"},
            {"prevrpt", "INTEGER"},
            {"detail", "INTEGER"},
            {"instance", "TEXT(40)"},
            {"nciks", "SMALLINT(4)"},
            {"aciks", "TEXT(120)"}
        }},
        {"tag", {
            {"tag", "TEXT(256)"},
            {"version", "TEXT(20)"},
            {"custom", "INTEGER"},
            {"abstract", "INTEGER"},
            {"datatype", "TEXT(20)"},
            {"iord", "TEXT(1)"},
            {"crdr", "TEXT(1)"},
            {"tlabel", "TEXT(512)"},
            {"doc", "TEXT"}
        }},
        {"num", {
            {"adsh", "TEXT(20)"},
            {"tag", "TEXT(256)"},
            {"version", "TEXT(20)"},
            {"ddate", "TEXT(8)"},
            {"qtrs", "SMALLINT(4)"},
            {"uom", "TEXT(20)"},
            {"segments", "TEXT(1024)"},
            {"coreg", "TEXT(256)"},
            {"value", "DECIMAL(28,4)"},
            {"footnote", "TEXT(512)"}
        }},
        {"pre", {
            {"adsh", "TEXT(20)"},
            {"report", "SMALLINT(3)"},
            {"line", "SMALLINT(5)"},
            {"stmt", "TEXT(2)"},
            {"inpth", "INTEGER"},
            {"rfile", "TEXT(1)"},
            {"tag", "TEXT(256)"},
            {"version", "TEXT(20)"},
            {"plabel", "TEXT(512)"},
            {"negating", "INTEGER"}
        }}
    };

    std::map<std::string, std::vector<std::string>> table_primary_keys = {
        {"sub", {"adsh"}},
        {"tag", {"tag", "version"}},
        {"num", {"adsh", "tag", "version", "ddate", "qtrs", "uom", "segments", "coreg"}},
        {"pre", {"adsh", "report", "line"}}
    };

    void create_directory(const std::string& dir) const {
        struct stat st = {0};
        if (stat(dir.c_str(), &st) == -1) {
            mkdir(dir.c_str(), 0755);
        }
    }

    std::set<std::string> get_existing_columns(const std::string& table) const {
        std::set<std::string> columns;
        sqlite3_stmt* stmt;
        std::string sql = "PRAGMA table_info(" + table + ");";
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return columns;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string col_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            columns.insert(col_name);
        }
        sqlite3_finalize(stmt);
        return columns;
    }

    void ensure_table_schema(const std::string& table, const std::vector<std::string>& file_headers) const {
        auto existing_cols = get_existing_columns(table);
        if (existing_cols.empty()) {
            std::string sql = "CREATE TABLE " + table + " (";
            const auto& col_types = table_column_types.at(table);
            for (const auto& col_type : col_types) {
                sql += col_type.first + " " + col_type.second + ", ";
            }
            sql += "PRIMARY KEY (";
            const auto& pks = table_primary_keys.at(table);
            for (const auto& pk : pks) {
                sql += pk + ", ";
            }
            if (!pks.empty()) {
                sql.pop_back(); sql.pop_back(); 
            }
            sql += "));";
            sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
            existing_cols = get_existing_columns(table); 
        }

        for (const auto& col : file_headers) {
            if (existing_cols.find(col) == existing_cols.end()) {
                const auto& col_types = table_column_types.at(table);
                auto it = col_types.find(col);
                std::string type;
                if (it != col_types.end()) {
                    type = it->second;
                } else {
                    type = "TEXT";
                    std::cerr << "Unknown column " << col << " in table " << table << ", using TEXT." << std::endl;
                }
                std::string alter_sql = "ALTER TABLE " + table + " ADD COLUMN " + col + " " + type + ";";
                sqlite3_exec(db, alter_sql.c_str(), nullptr, nullptr, nullptr);
            }
        }
    }

    void init_db() {
        create_directory(base_dir);
        sqlite3_open(db_file.c_str(), &db);

        std::string sql = "CREATE TABLE IF NOT EXISTS processed_quarters (quarter TEXT PRIMARY KEY, processed_date DATETIME DEFAULT CURRENT_TIMESTAMP);";
        sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);

        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_sub_cik ON sub(cik)", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_num_adsh ON num(adsh)", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_num_tag ON num(tag)", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_num_ddate ON num(ddate)", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_num_qtrs_uom_segments ON num(qtrs, uom, segments)", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_num_cik_tag_ddate ON num(adsh, tag, ddate DESC)", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_sub_adsh_cik ON sub(adsh, cik)", nullptr, nullptr, nullptr);
    }

    bool file_exists(const std::string& file) const {
        struct stat buffer;
        return (stat(file.c_str(), &buffer) == 0);
    }

    bool is_quarter_processed(const std::string& quarter) const {
        sqlite3_stmt* stmt;
        std::string sql = "SELECT 1 FROM processed_quarters WHERE quarter = ?;";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, quarter.c_str(), -1, SQLITE_STATIC);
        bool processed = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return processed;
    }

    void mark_quarter_processed(const std::string& quarter) const {
        sqlite3_stmt* stmt;
        std::string sql = "INSERT OR REPLACE INTO processed_quarters (quarter) VALUES (?);";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, quarter.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::ofstream*)userp)->write((char*)contents, size * nmemb);
        return size * nmemb;
    }

    bool download_zip(const std::string& quarter) const {
        std::string url = "https://www.sec.gov/files/dera/data/financial-statement-data-sets/" + quarter + ".zip";
        std::string zip_file = base_dir + quarter + ".zip";
        CURL* curl = curl_easy_init();
        if (curl) {
            std::ofstream f(zip_file, std::ios::binary);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "FinancialProcessor/1.0 (nlane112358@gmail.com)");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &f);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res != CURLE_OK) {
                std::cerr << "Download failed for " << quarter << ": " << curl_easy_strerror(res) << std::endl;
                return false;
            }
            std::ifstream check(zip_file);
            std::string first_line;
            std::getline(check, first_line);
            if (first_line.find("<!DOCTYPE html") != std::string::npos) {
                std::remove(zip_file.c_str());
                return false;
            }
            return true;
        }
        return false;
    }

    bool extract_zip(const std::string& quarter) const {
        std::string zip_file = base_dir + quarter + ".zip";
        std::string out_dir = base_dir + quarter + "/";
        create_directory(out_dir);
        unzFile uf = unzOpen(zip_file.c_str());
        if (!uf) return false;
        do {
            char filename[256];
            unz_file_info file_info;
            if (unzGetCurrentFileInfo(uf, &file_info, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK) break;
            if (unzOpenCurrentFile(uf) != UNZ_OK) break;
            std::ofstream out(out_dir + filename, std::ios::binary);
            std::vector<char> buffer(4096);
            while (true) {
                int len = unzReadCurrentFile(uf, buffer.data(), buffer.size());
                if (len <= 0) break;
                out.write(buffer.data(), len);
            }
            unzCloseCurrentFile(uf);
        } while (unzGoToNextFile(uf) == UNZ_OK);
        unzClose(uf);
        return true;
    }

    void parse_and_insert(const std::string& tsv_file, const std::string& table) const {
        std::ifstream f(tsv_file);
        if (!f.is_open()) {
            std::cerr << "File not found: " << tsv_file << std::endl;
            return;
        }
        std::string header_line;
        std::getline(f, header_line);
        std::stringstream ss(header_line);
        std::string token;
        std::vector<std::string> headers;
        while (std::getline(ss, token, '\t')) headers.push_back(token);

        ensure_table_schema(table, headers);

        int num_columns = headers.size();
        int max_params = sqlite3_limit(db, SQLITE_LIMIT_VARIABLE_NUMBER, -1);
        int batch_size = std::min(100, max_params / num_columns);
        if (batch_size < 1) batch_size = 1;

        std::string col_list;
        for (const auto& h : headers) {
            col_list += h + ",";
        }
        col_list.pop_back();

        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
        std::vector<std::vector<std::string>> batch;
        std::string line;
        int read_lines = 0;
        int skipped = 0;
        int padded = 0;
        while (std::getline(f, line)) {
            read_lines++;
            std::stringstream ss_line(line);
            std::vector<std::string> row;
            while (std::getline(ss_line, token, '\t')) row.push_back(token);
            int original_size = row.size();
            while (row.size() < num_columns) {
                row.push_back("");
            }
            if (original_size < num_columns) {
                padded++;
                std::cout << "Padded row " << read_lines << " from " << original_size << " to " << num_columns << " columns in " << table << std::endl;
            }
            if (row.size() > num_columns) {
                skipped++;
                std::cerr << "Skipped row " << read_lines << " with " << row.size() << " columns (expected " << num_columns << ") in " << table << std::endl;
                continue;
            }
            batch.push_back(row);
            if (batch.size() >= batch_size) {
                insert_batch(table, batch, headers, col_list);
                batch.clear();
            }
        }
        if (!batch.empty()) insert_batch(table, batch, headers, col_list);
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
        std::cout << "Read " << read_lines << " body lines, padded " << padded << ", skipped " << skipped << " for table " << table << std::endl;
    }

    void insert_batch(const std::string& table, const std::vector<std::vector<std::string>>& batch, const std::vector<std::string>& headers, const std::string& col_list) const {
        if (batch.empty()) return;
        std::string sql = "INSERT OR IGNORE INTO " + table + " (" + col_list + ") VALUES ";
        for (size_t i = 0; i < batch.size(); ++i) {
            sql += "(";
            for (size_t j = 0; j < headers.size(); ++j) sql += "?,";
            sql.back() = ')';
            if (i < batch.size() - 1) sql += ",";
        }
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Prepare failed for batch in " << table << ": " << sqlite3_errmsg(db) << std::endl;
            return;
        }
        int param_index = 1;
        for (const auto& row : batch) {
            for (const auto& value : row) {
                sqlite3_bind_text(stmt, param_index++, value.c_str(), -1, SQLITE_STATIC);
            }
        }
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Insert failed for batch in " << table << ": " << sqlite3_errmsg(db) << std::endl;
        } else {
            int changed = sqlite3_changes(db);
            std::cout << "Inserted " << changed << " rows in batch for " << table << std::endl;
        }
        sqlite3_finalize(stmt);
    }

    int get_row_count(const std::string& table) const {
        sqlite3_stmt* stmt;
        std::string sql = "SELECT COUNT(*) FROM " + table + ";";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return count;
    }

    std::string get_processed_quarters() const {
        sqlite3_stmt* stmt;
        std::string sql = "SELECT GROUP_CONCAT(quarter, ', ') FROM processed_quarters;";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        std::string quarters = "";
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (text) quarters = text;
        }
        sqlite3_finalize(stmt);
        return quarters;
    }

public:
    FinancialProcessor() {
        init_db();
    }

    ~FinancialProcessor() {
        sqlite3_close(db);
    }

    void process_quarter(const std::string& quarter) const {
        if (is_quarter_processed(quarter)) {
            std::cout << "Quarter " << quarter << " already processed." << std::endl;
            return;
        }
        if (!download_zip(quarter)) {
            std::cout << "Failed to download " << quarter << std::endl;
            return;
        }
        if (!extract_zip(quarter)) {
            std::cout << "Failed to extract " << quarter << std::endl;
            return;
        }
        std::string out_dir = base_dir + quarter + "/";
        parse_and_insert(out_dir + "sub.txt", "sub");
        parse_and_insert(out_dir + "tag.txt", "tag");
        parse_and_insert(out_dir + "num.txt", "num");
        parse_and_insert(out_dir + "pre.txt", "pre");
        mark_quarter_processed(quarter);
        std::cout << "Processed " << quarter << std::endl;
    }

    void process_all_quarters() const {
        std::vector<std::string> quarters;
        for (int year = 2009; year <= 2025; ++year) {
            for (int q = 1; q <= 4; ++q) {
                if (year == 2025 && q > 2) break;
                quarters.push_back(std::to_string(year) + "q" + std::to_string(q));
            }
        }
        for (const auto& q : quarters) {
            process_quarter(q);
        }
    }

    std::map<std::string, double> query_fundamentals(int cik, const std::string& tag) const {
        std::map<std::string, double> results;
        sqlite3_stmt* stmt;
        std::string sql = "SELECT n.value, n.ddate FROM num n JOIN sub s ON n.adsh = s.adsh WHERE s.cik = ? AND n.tag = ? ORDER BY n.ddate DESC LIMIT 1";
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
            return results;
        }
        sqlite3_bind_int(stmt, 1, cik);
        sqlite3_bind_text(stmt, 2, tag.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double value = sqlite3_column_double(stmt, 0);
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            results[date] = value;
        }
        sqlite3_finalize(stmt);
        if (results.empty()) {
            std::cout << "No data found for CIK " << cik << " and tag '" << tag << "'." << std::endl;
        }
        return results;
    }

    std::map<std::string, double> query_time_series(int cik, const std::string& tag, int qtrs, const std::string& uom, const std::string& start_date, const std::string& end_date) const {
        std::map<std::string, double> results;
        sqlite3_stmt* stmt;
        std::string sql = "SELECT n.ddate, n.value FROM num n JOIN sub s ON n.adsh = s.adsh WHERE s.cik = ? AND n.tag = ? AND n.qtrs = ? AND n.uom = ? AND n.ddate BETWEEN ? AND ? ORDER BY n.ddate";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, cik);
        sqlite3_bind_text(stmt, 2, tag.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, qtrs);
        sqlite3_bind_text(stmt, 4, uom.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, start_date.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, end_date.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            double value = sqlite3_column_double(stmt, 1);
            results[date] = value;
        }
        sqlite3_finalize(stmt);
        return results;
    }
    
    std::vector<std::map<std::string, std::string>> execute_custom_query(const std::string& sql) const {
        std::vector<std::map<std::string, std::string>> results;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
            return results;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::map<std::string, std::string> row;
            int num_cols = sqlite3_column_count(stmt);
            for (int i = 0; i < num_cols; ++i) {
                const char* col_name = sqlite3_column_name(stmt, i);
                const char* col_value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                row[col_name] = col_value ? col_value : "";
            }
            results.push_back(row);
        }
        sqlite3_finalize(stmt);
        return results;
    }

    std::map<int, std::map<std::string, std::pair<std::string, double>>> query_all_latest_fundamentals(
        const std::vector<std::string>& tags, 
        int qtrs, 
        const std::string& uom, 
        const std::string& segments
    ) const {
        std::map<int, std::map<std::string, std::pair<std::string, double>>> results;
        
        if (tags.empty()) return results;
        
        std::string tags_in = "(";
        for (const auto& t : tags) {
            tags_in += "'" + t + "',";
        }
        tags_in.back() = ')'; 
        
        std::string sql = R"(
            WITH latest AS (
                SELECT s.cik, n.tag, n.ddate, n.value,
                    ROW_NUMBER() OVER (PARTITION BY s.cik, n.tag ORDER BY n.ddate DESC) AS rn
                FROM num n
                JOIN sub s ON n.adsh = s.adsh
                WHERE n.tag IN )" + tags_in + R"(
        )";
        if (qtrs != -1) sql += " AND n.qtrs = " + std::to_string(qtrs);
        if (!uom.empty()) sql += " AND n.uom = '" + uom + "'";
        if (!segments.empty()) sql += " AND n.segments = '" + segments + "'";
        sql += R"(
                )
            SELECT cik, tag, ddate, value
            FROM latest
            WHERE rn = 1;  -- Latest per CIK/tag
        )";
        
        auto rows = execute_custom_query(sql); 
        
        for (const auto& row : rows) {
            int cik = std::stoi(row.at("cik"));
            std::string tag = row.at("tag");
            std::string ddate = row.at("ddate");
            double value = std::stod(row.at("value"));
            results[cik][tag] = {ddate, value};
        }
        
        if (rows.empty()) {
            std::cout << "No data found for the specified tags and filters." << std::endl;
        }
        
        return results;
    }

    void print_db_summary() const {
        std::cout << "Database Summary for " << db_file << ":" << std::endl;
        std::cout << "Number of submissions (sub table): " << get_row_count("sub") << std::endl;
        std::cout << "Number of tags (tag table): " << get_row_count("tag") << std::endl;
        std::cout << "Number of numeric facts (num table): " << get_row_count("num") << std::endl;
        std::cout << "Number of presentation entries (pre table): " << get_row_count("pre") << std::endl;
        int quarter_count = get_row_count("processed_quarters");
        std::cout << "Number of processed quarters: " << quarter_count << std::endl;
        std::string quarters = get_processed_quarters();
        if (!quarters.empty()) {
            std::cout << "Processed quarters: " << quarters << std::endl;
        }
        sqlite3_stmt* stmt;
        std::string sql = "SELECT COUNT(DISTINCT cik) FROM sub;";
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        int unique_ciks = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            unique_ciks = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        std::cout << "Number of unique CIKs: " << unique_ciks << std::endl;
    }
};

#endif
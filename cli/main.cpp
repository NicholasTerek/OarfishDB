#include "src/include/planner/database.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {
void print_result(const oarfish::QueryResult& result) {
    std::vector<std::size_t> widths;
    for (const auto& column : result.columns) widths.push_back(column.size());
    for (const auto& row : result.rows) {
        for (std::size_t i = 0; i < row.size(); ++i)
            widths[i] = std::max(widths[i], row[i].size());
    }
    auto print_row = [&](const std::vector<std::string>& row) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i) std::cout << " | ";
            std::cout << std::left << std::setw(widths[i]) << row[i];
        }
        std::cout << '\n';
    };
    print_row(result.columns);
    for (const auto& row : result.rows) print_row(row);
    std::cout << '(' << result.rows.size() << (result.rows.size() == 1 ? " row)\n" : " rows)\n");
}
}

int main(int argc, char** argv) {
    const bool demo = argc == 2 && std::string(argv[1]) == "--demo";
    if (argc != 1 && !demo) {
        std::cerr << "Usage: oarfishdb [--demo]\n";
        return 1;
    }
    Lsm storage;
    std::size_t count = 0;
    if (demo) {
        storage.put("index", "Bloom filter");
        storage.put("memory", "Skip list");
        storage.put("storage", "LSM tree");
        count = 3;
    }
    oarfish::Database db(storage, {count});
    std::cout << "OarfishDB  |  " << (demo ? "demo session" : "temporary session") << "\n";
    std::string line;
    while (std::cout << "oarfish> " << std::flush, std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string command;
        if (!(input >> command)) continue;
        try {
            if (command == ".quit") break;
            if (command == ".put") {
                std::string key, value;
                input >> key;
                if (!std::getline(input >> std::ws, value) || value.empty())
                    throw std::invalid_argument("Usage: .put KEY VALUE");
                const bool exists = storage.get(key).has_value();
                storage.put(key, value);
                if (!exists) db.set_statistics({++count});
                std::cout << "OK\n";
            } else if (command == ".explain") {
                std::string sql;
                std::getline(input, sql);
                const auto plan = db.explain(sql);
                std::cout << (plan.access_path == oarfish::AccessPath::PointLookup
                    ? "Point lookup" : "Full scan") << " on kv  |  estimated rows: "
                    << plan.estimated_rows << '\n';
            } else {
                print_result(db.query(line));
            }
        } catch (const std::exception& error) {
            std::cerr << "Error: " << error.what() << '\n';
        }
    }
}

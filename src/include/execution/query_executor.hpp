#pragma once
#include "src/include/optimizer/cost_model.hpp"
#include "src/include/storage/lsm_storage.hpp"
#include <string>
#include <vector>

namespace oarfish {

struct QueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
};

class QueryExecutor {
public:
    explicit QueryExecutor(Lsm& storage);
    QueryResult execute(const QueryPlan& plan);

private:
    Lsm& storage_;
};

}

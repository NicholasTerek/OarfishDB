#pragma once
#include "src/include/execution/query_executor.hpp"
#include <string_view>

namespace oarfish {

class Database {
public:
    explicit Database(Lsm& storage, TableStatistics statistics = {});
    QueryPlan explain(std::string_view sql) const;
    QueryResult query(std::string_view sql);
    void set_statistics(TableStatistics statistics);

private:
    Lsm& storage_;
    TableStatistics statistics_;
};

}

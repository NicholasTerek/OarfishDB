#include "src/include/planner/database.hpp"
#include "src/include/planner/parser.hpp"
#include <utility>

namespace oarfish {

Database::Database(Lsm& storage, TableStatistics statistics)
    : storage_(storage), statistics_(statistics) {}

QueryPlan Database::explain(std::string_view sql) const {
    return optimize(parse_select(sql), statistics_);
}

QueryResult Database::query(std::string_view sql) {
    return QueryExecutor(storage_).execute(explain(sql));
}

void Database::set_statistics(TableStatistics statistics) {
    statistics_ = std::move(statistics);
}

}

#include "src/include/execution/query_executor.hpp"
#include <stdexcept>
#include <utility>

namespace oarfish {
namespace {

bool compare(const std::string& lhs, Comparison op, const std::string& rhs) {
    switch (op) {
        case Comparison::Equal: return lhs == rhs;
        case Comparison::NotEqual: return lhs != rhs;
        case Comparison::Less: return lhs < rhs;
        case Comparison::LessEqual: return lhs <= rhs;
        case Comparison::Greater: return lhs > rhs;
        case Comparison::GreaterEqual: return lhs >= rhs;
    }
    throw std::invalid_argument("Invalid comparison");
}

bool matches(const Predicate& predicate, const std::string& key, const std::string& value) {
    const auto& field = predicate.column == "key" ? key : value;
    return compare(field, predicate.op, predicate.value);
}

std::vector<std::string> project(const std::vector<std::string>& columns,
                                 const std::string& key, const std::string& value) {
    std::vector<std::string> row;
    row.reserve(columns.size());
    for (const auto& column : columns) {
        row.push_back(column == "key" ? key : value);
    }
    return row;
}

}

QueryExecutor::QueryExecutor(Lsm& storage) : storage_(storage) {}

QueryResult QueryExecutor::execute(const QueryPlan& plan) {
    const auto& query = plan.query;
    if (query.table != "kv") throw std::invalid_argument("Unknown table: " + query.table);

    auto columns = query.columns;
    if (columns.size() == 1 && columns.front() == "*") columns = {"key", "value"};
    for (const auto& column : columns) {
        if (column != "key" && column != "value") {
            throw std::invalid_argument("Unknown column: " + column);
        }
    }
    if (query.predicate && query.predicate->column != "key" && query.predicate->column != "value") {
        throw std::invalid_argument("Unknown predicate column");
    }

    QueryResult result{std::move(columns), {}};
    if (query.limit && *query.limit == 0) return result;

    auto append = [&](const std::string& key, const std::string& value) {
        if (query.predicate && !matches(*query.predicate, key, value)) return;
        result.rows.push_back(project(result.columns, key, value));
    };

    if (plan.access_path == AccessPath::PointLookup) {
        if (!query.predicate || query.predicate->column != "key" || query.predicate->op != Comparison::Equal) {
            throw std::invalid_argument("Point lookup requires equality on key");
        }
        const auto& key = query.predicate->value;
        const auto value = storage_.get(key);
        if (value) append(key, *value);
        return result;
    }

    auto iterator = storage_.scan();
    while (iterator->is_valid()) {
        append(iterator->key(), iterator->value());
        if (query.limit && result.rows.size() >= *query.limit) break;
        iterator->next();
    }
    return result;
}

}

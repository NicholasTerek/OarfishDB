#include "src/include/optimizer/cost_model.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace oarfish {

QueryPlan optimize(SelectQuery query, const TableStatistics& statistics) {
    if (statistics.estimated_point_read_cost < 0 || statistics.estimated_row_scan_cost < 0) {
        throw std::invalid_argument("Costs must be nonnegative");
    }

    QueryPlan plan;
    plan.query = std::move(query);
    plan.estimated_rows = static_cast<double>(statistics.row_count);

    const bool has_key_equality = plan.query.predicate &&
        plan.query.predicate->column == "key" &&
        plan.query.predicate->op == Comparison::Equal;
    const double scan_cost = statistics.row_count * statistics.estimated_row_scan_cost;

    if (has_key_equality && statistics.estimated_point_read_cost <= scan_cost) {
        plan.access_path = AccessPath::PointLookup;
        plan.estimated_cost = statistics.estimated_point_read_cost;
        plan.estimated_rows = std::min(1.0, plan.estimated_rows);
    } else {
        plan.estimated_cost = scan_cost;
        if (plan.query.predicate) plan.estimated_rows *= 0.1;
    }

    if (plan.query.limit) {
        plan.estimated_rows = std::min(plan.estimated_rows, static_cast<double>(*plan.query.limit));
    }
    return plan;
}

}

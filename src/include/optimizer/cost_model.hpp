#pragma once
#include "src/include/planner/query.hpp"
#include <cstddef>

namespace oarfish {

enum class AccessPath { FullScan, PointLookup };

struct TableStatistics {
    std::size_t row_count = 0;
    double estimated_point_read_cost = 3.0;
    double estimated_row_scan_cost = 1.0;
};

struct QueryPlan {
    SelectQuery query;
    AccessPath access_path = AccessPath::FullScan;
    double estimated_cost = 0.0;
    double estimated_rows = 0.0;
};

QueryPlan optimize(SelectQuery query, const TableStatistics& statistics);

}

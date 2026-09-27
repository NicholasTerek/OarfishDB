#include "src/include/optimizer/cost_model.hpp"
#include <gtest/gtest.h>

TEST(QueryOptimizerTest, ChoosesPointLookupWhenCheaper) {
    oarfish::SelectQuery query;
    query.table = "kv";
    query.columns = {"*"};
    query.predicate = oarfish::Predicate{"key", oarfish::Comparison::Equal, "item"};
    const auto plan = oarfish::optimize(query, {100, 3.0, 1.0});
    EXPECT_EQ(plan.access_path, oarfish::AccessPath::PointLookup);
    EXPECT_DOUBLE_EQ(plan.estimated_cost, 3.0);
    EXPECT_DOUBLE_EQ(plan.estimated_rows, 1.0);
}

TEST(QueryOptimizerTest, KeepsScanWhenPointLookupCostsMore) {
    oarfish::SelectQuery query;
    query.table = "kv";
    query.columns = {"*"};
    query.predicate = oarfish::Predicate{"key", oarfish::Comparison::Equal, "item"};
    const auto plan = oarfish::optimize(query, {2, 10.0, 1.0});
    EXPECT_EQ(plan.access_path, oarfish::AccessPath::FullScan);
    EXPECT_DOUBLE_EQ(plan.estimated_cost, 2.0);
}

TEST(QueryOptimizerTest, UsesScanForValuePredicates) {
    oarfish::SelectQuery query;
    query.table = "kv";
    query.columns = {"value"};
    query.predicate = oarfish::Predicate{"value", oarfish::Comparison::Equal, "hello"};
    const auto plan = oarfish::optimize(query, {100, 1.0, 1.0});
    EXPECT_EQ(plan.access_path, oarfish::AccessPath::FullScan);
}

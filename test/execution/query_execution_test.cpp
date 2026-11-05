#include "src/include/planner/database.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class QueryExecutionTest : public ::testing::Test {
protected:
    QueryExecutionTest() : database(storage, {3}) {
        storage.put("foo", "bar");
        storage.put("baz", "qux");
        storage.put("quux", "bar");
    }

    Lsm storage;
    oarfish::Database database;
};

TEST_F(QueryExecutionTest, FindsValueByKey) {
    const auto result = database.query("SELECT value FROM kv WHERE key = 'foo'");

    EXPECT_EQ(result.columns, (std::vector<std::string>{"value"}));
    ASSERT_EQ(result.rows.size(), 1U);
    EXPECT_EQ(result.rows[0], (std::vector<std::string>{"bar"}));
}

TEST_F(QueryExecutionTest, ScansAllRecords) {
    const auto result = database.query("SELECT * FROM kv");

    EXPECT_EQ(result.columns, (std::vector<std::string>{"key", "value"}));
    EXPECT_EQ(result.rows, (std::vector<std::vector<std::string>>{
        {"baz", "qux"},
        {"foo", "bar"},
        {"quux", "bar"}
    }));
}

TEST_F(QueryExecutionTest, ProjectsRequestedColumns) {
    const auto result = database.query("SELECT key FROM kv WHERE key = 'foo'");

    EXPECT_EQ(result.columns, (std::vector<std::string>{"key"}));
    ASSERT_EQ(result.rows.size(), 1U);
    EXPECT_EQ(result.rows[0], (std::vector<std::string>{"foo"}));
}

TEST_F(QueryExecutionTest, FiltersByValue) {
    const auto result = database.query("SELECT key FROM kv WHERE value = 'bar'");

    EXPECT_EQ(result.rows, (std::vector<std::vector<std::string>>{
        {"foo"},
        {"quux"}
    }));
}

TEST_F(QueryExecutionTest, SupportsRangePredicates) {
    const auto result = database.query("SELECT key FROM kv WHERE key > 'foo'");

    EXPECT_EQ(result.rows, (std::vector<std::vector<std::string>>{{"quux"}}));
}

TEST_F(QueryExecutionTest, AppliesLimit) {
    const auto result = database.query("SELECT key FROM kv LIMIT 2");

    EXPECT_EQ(result.rows, (std::vector<std::vector<std::string>>{
        {"baz"},
        {"foo"}
    }));
}

TEST_F(QueryExecutionTest, ReturnsNoRowsForZeroLimit) {
    const auto result = database.query("SELECT * FROM kv LIMIT 0");

    EXPECT_TRUE(result.rows.empty());
}

TEST_F(QueryExecutionTest, ReturnsNoRowsForMissingKey) {
    const auto result = database.query("SELECT * FROM kv WHERE key = 'missing'");

    EXPECT_TRUE(result.rows.empty());
}

TEST_F(QueryExecutionTest, RejectsUnknownColumn) {
    EXPECT_THROW(database.query("SELECT missing FROM kv"), std::invalid_argument);
}

TEST_F(QueryExecutionTest, RejectsUnknownTable) {
    EXPECT_THROW(database.query("SELECT * FROM missing"), std::invalid_argument);
}

TEST_F(QueryExecutionTest, ExplainsPointLookup) {
    const auto plan = database.explain("SELECT * FROM kv WHERE key = 'foo'");

    EXPECT_EQ(plan.access_path, oarfish::AccessPath::PointLookup);
    EXPECT_DOUBLE_EQ(plan.estimated_rows, 1.0);
}

TEST_F(QueryExecutionTest, ExplainsValueFilterAsScan) {
    const auto plan = database.explain("SELECT * FROM kv WHERE value = 'bar'");

    EXPECT_EQ(plan.access_path, oarfish::AccessPath::FullScan);
}

TEST_F(QueryExecutionTest, ChoosesScanWhenEstimatedCheaper) {
    database.set_statistics({3, 10.0, 1.0});

    const auto plan = database.explain("SELECT * FROM kv WHERE key = 'foo'");
    EXPECT_EQ(plan.access_path, oarfish::AccessPath::FullScan);

    const auto result = database.query("SELECT value FROM kv WHERE key = 'foo'");
    ASSERT_EQ(result.rows.size(), 1U);
    EXPECT_EQ(result.rows[0], (std::vector<std::string>{"bar"}));
}

}

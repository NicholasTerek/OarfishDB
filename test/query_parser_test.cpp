#include "src/include/planner/parser.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

TEST(QueryParserTest, ParsesProjectionAndSource) {
    const auto query = oarfish::parse_select("SELECT key, value FROM kv;");
    EXPECT_EQ(query.table, "kv");
    EXPECT_EQ(query.columns, (std::vector<std::string>{"key", "value"}));
    EXPECT_FALSE(query.predicate.has_value());
}

TEST(QueryParserTest, ParsesPredicateAndLimit) {
    const auto query = oarfish::parse_select("select value from kv where key = 'a' limit 2");
    ASSERT_TRUE(query.predicate.has_value());
    EXPECT_EQ(query.predicate->column, "key");
    EXPECT_EQ(query.predicate->op, oarfish::Comparison::Equal);
    EXPECT_EQ(query.predicate->value, "a");
    ASSERT_TRUE(query.limit.has_value());
    EXPECT_EQ(*query.limit, 2U);
}

TEST(QueryParserTest, EscapesSingleQuotes) {
    const auto query = oarfish::parse_select("SELECT * FROM kv WHERE value = 'it''s fine'");
    ASSERT_TRUE(query.predicate.has_value());
    EXPECT_EQ(query.predicate->value, "it's fine");
}

TEST(QueryParserTest, RejectsIncompleteQuery) {
    EXPECT_THROW(oarfish::parse_select("SELECT FROM kv"), std::invalid_argument);
    EXPECT_THROW(oarfish::parse_select("SELECT key FROM"), std::invalid_argument);
    EXPECT_THROW(oarfish::parse_select("SELECT * FROM kv WHERE key ="), std::invalid_argument);
}

TEST(QueryParserTest, RejectsUnsupportedStatements) {
    EXPECT_THROW(oarfish::parse_select("DELETE FROM kv"), std::invalid_argument);
    EXPECT_THROW(oarfish::parse_select("SELECT * FROM kv ORDER BY key"), std::invalid_argument);
}

#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace oarfish {

enum class Comparison { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };

struct Predicate {
    std::string column;
    Comparison op;
    std::string value;
};

struct SelectQuery {
    std::vector<std::string> columns;
    std::string table;
    std::optional<Predicate> predicate;
    std::optional<std::size_t> limit;
};

}

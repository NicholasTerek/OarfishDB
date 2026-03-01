#pragma once

#include <string>

enum class BoundType {
    Unbounded,
    Included,
    Excluded,
};

struct Bound {
    BoundType type;
    std::string key;

    static Bound unbounded() { return Bound{BoundType::Unbounded, ""}; }
    static Bound included(const std::string& k) { return Bound{BoundType::Included, k}; }
    static Bound excluded(const std::string& k) { return Bound{BoundType::Excluded, k}; }
};

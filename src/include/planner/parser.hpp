#pragma once
#include "query.hpp"
#include <string_view>

namespace oarfish {

SelectQuery parse_select(std::string_view sql);

}

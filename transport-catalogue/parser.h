#pragma once

#include <string>
#include <utility>
#include <vector>

namespace transport_catalogue {

std::vector<std::pair<int, std::string>> ParseStopDistances(const std::string& input);

} // namespace transport_catalogue

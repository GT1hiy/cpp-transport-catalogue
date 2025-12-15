#include "parser.h"
#include <sstream>
#include <stdexcept>

namespace transport_catalogue {

std::vector<std::pair<int, std::string>> ParseStopDistances(const std::string& input) {
    std::vector<std::pair<int, std::string>> result;

    size_t dist_pos = input.find(',', input.find(',') + 1);
    if (dist_pos == std::string::npos) {
        return result;
    }

    std::string distances_str = input.substr(dist_pos);
    std::istringstream iss(distances_str);
    std::string token;

    while (getline(iss, token, ',')) {
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);

        if (token.empty()) continue;

        size_t m_pos = token.find('m');
        if (m_pos == std::string::npos) {
            throw std::invalid_argument("Invalid distance format - missing 'm'");
        }

        std::string dist_str = token.substr(0, m_pos);
        int distance;
        try {
            distance = std::stoi(dist_str);
        } catch (...) {
            throw std::invalid_argument("Invalid distance value");
        }

        if (distance <= 0) {
            throw std::invalid_argument("Distance must be positive");
        }

        size_t to_pos = token.find("to ", m_pos);
        if (to_pos == std::string::npos) {
            throw std::invalid_argument("Invalid format - missing 'to'");
        }

        std::string stop_name = token.substr(to_pos + 3);
        stop_name.erase(0, stop_name.find_first_not_of(" \t"));
        stop_name.erase(stop_name.find_last_not_of(" \t") + 1);

        if (stop_name.empty()) {
            throw std::invalid_argument("Empty stop name");
        }

        result.emplace_back(distance, stop_name);
    }

    return result;
}

} // namespace transport_catalogue

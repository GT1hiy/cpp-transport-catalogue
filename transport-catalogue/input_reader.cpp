#include "input_reader.h"
#include <cassert>
#include <iterator>
#include <algorithm>
#include <regex>
#include <iostream>

namespace input {

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

/**
 * Парсит информацию о расстояниях из строки вида "3900m to Marushkino, 100m to Stop2"
 */
std::vector<std::pair<std::string, int>> ParseDistances(std::string_view distances_str) {
    std::vector<std::pair<std::string, int>> result;
    
    if (distances_str.empty()) {
        return result;
    }
    
    // Разбиваем по запятым
    auto distance_items = Split(distances_str, ',');
    
    for (const auto& item : distance_items) {
        // Находим "m to "
        auto m_pos = item.find("m to ");
        if (m_pos == std::string_view::npos) {
            continue;
        }
        
        // Парсим расстояние
        std::string dist_str(item.substr(0, m_pos));
        int distance = std::stoi(dist_str);
        
        // Парсим название остановки
        std::string stop_name(item.substr(m_pos + 5));
        
        result.emplace_back(std::move(stop_name), distance);
    }
    
    return result;
}

/**
 * Извлекает координаты из строки описания остановки
 */
std::pair<geo::Coordinates, std::string_view> ExtractCoordinatesAndDistances(std::string_view description) {
    auto first_comma = description.find(',');
    if (first_comma == std::string_view::npos) {
        return {ParseCoordinates(description), {}};
    }
    
    auto second_comma = description.find(',', first_comma + 1);
    if (second_comma == std::string_view::npos) {
        // Только координаты, без расстояний
        return {ParseCoordinates(description), {}};
    }
    
    // Координаты
    std::string_view coords_str = description.substr(0, second_comma);
    geo::Coordinates coords = ParseCoordinates(coords_str);
    
    // Расстояния
    std::string_view distances_str = description.substr(second_comma + 1);
    
    return {coords, distances_str};
}

input::CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void input::Reader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void input::Reader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) {
    // Первый проход: добавляем все остановки (только координаты)
    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            auto [coords, distances_str] = ExtractCoordinatesAndDistances(command.description);
            catalogue.AddStop(command.id, coords);
        }
    }
    
    // Второй проход: добавляем расстояния
    for (const auto& command : commands_) {
        if (command.command == "Stop") {
            auto [coords, distances_str] = ExtractCoordinatesAndDistances(command.description);
            
            if (!distances_str.empty()) {
                // Парсим и добавляем расстояния
                auto distances = ParseDistances(distances_str);
                for (const auto& [to_stop, distance] : distances) {
                    catalogue.SetDistance(command.id, to_stop, distance);
                }
            }
        }
    }
    
    // Третий проход: добавляем автобусы
    for (const auto& command : commands_) {
        if (command.command == "Bus") {
            auto stops = ParseRoute(command.description);
            bool is_roundtrip = command.description.find('>') != std::string::npos;

            std::vector<std::string> stop_names;
            stop_names.reserve(stops.size());
            for (auto stop : stops) {
                stop_names.emplace_back(stop);
            }

            catalogue.AddBus(command.id, std::move(stop_names), is_roundtrip);
        }
    }
}

} // namespace input

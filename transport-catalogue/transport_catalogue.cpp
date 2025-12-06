#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <optional>
#include <cassert>
#include "transport_catalogue.h"

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string& name, Coordinates coordinates) {
    all_stops_.push_back({name, coordinates});
    stopname_to_stop_[all_stops_.back().name] = &all_stops_.back();
    stop_to_buses_[all_stops_.back().name];
}

void TransportCatalogue::AddBus(const std::string& name_number, const std::vector<std::string>& stops, bool is_roundtrip) {
    std::vector<const Stop*> bus_stops;
    bus_stops.reserve(stops.size());

    for (const auto& stop_name : stops) {
        if (auto it = stopname_to_stop_.find(stop_name); it != stopname_to_stop_.end()) {
            bus_stops.push_back(it->second);
        }
    }

    all_buses_.push_back({name_number, std::move(bus_stops), is_roundtrip});
    busname_to_bus_[all_buses_.back().name] = &all_buses_.back();

    UpdateStopToBus(name_number, stops);
}

void TransportCatalogue::UpdateStopToBus(const std::string& name_number, const std::vector<std::string>& stops) {
    const Bus* bus_ptr = busname_to_bus_.at(name_number);

    for (const auto& stop_name : stops) {
        if (auto it = stopname_to_stop_.find(stop_name); it != stopname_to_stop_.end()) {
            stop_to_buses_[stop_name].insert(bus_ptr);
        }
    }
}

void TransportCatalogue::SetDistance(const std::string& from, const std::string& to, int distance) {
    auto from_stop = GetStop(from);
    auto to_stop = GetStop(to);
    
    if (from_stop && to_stop) {
        distances_[{from_stop, to_stop}] = distance;
    }
}

int TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
    if (from == nullptr || to == nullptr) {
        return 0;
    }
    
    if (from == to) {
        return 0;
    }
    
    // Прямой поиск
    auto direct_it = distances_.find({from, to});
    if (direct_it != distances_.end()) {
        return direct_it->second;
    }
    
    // Обратный поиск
    auto reverse_it = distances_.find({to, from});
    if (reverse_it != distances_.end()) {
        return reverse_it->second;
    }
    
    return 0;
}

int TransportCatalogue::GetDistance(const std::string& from, const std::string& to) const {
    auto from_stop = GetStop(from);
    auto to_stop = GetStop(to);
    return GetDistance(from_stop, to_stop);
}

const Bus* TransportCatalogue::GetBus(std::string_view name_number) const {
    auto it = busname_to_bus_.find(name_number);
    if (it != busname_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const Stop* TransportCatalogue::GetStop(std::string_view name) const {
    auto it = stopname_to_stop_.find(name);
    if (it != stopname_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

std::set<const Bus*> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
        return it->second;
    }
    return {};
}

std::optional<RouteInfo> TransportCatalogue::RouteInformation(std::string_view number_name) const {
    const Bus* bus = GetBus(number_name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }

    RouteInfo info{0, 0, 0.0, 0.0};

    // Вычисляем общее количество остановок
    info.stops_count = bus->is_roundtrip ? bus->stops.size() : bus->stops.size() * 2 - 1;

    // Вычисляем количество уникальных остановок
    std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stops_count = unique_stops.size();

    // Вычисляем географическую длину маршрута
    double geo_length = 0.0;
    // Вычисляем дорожную длину маршрута
    double road_length = 0.0;
    
    // Для кольцевого маршрута
    if (bus->is_roundtrip) {
        for (size_t i = 0; i < bus->stops.size(); ++i) {
            size_t next_i = (i + 1) % bus->stops.size();
            
            // Географическое расстояние
            geo_length += geo::ComputeDistance(bus->stops[i]->coordinates,
                                             bus->stops[next_i]->coordinates);
            
            // Дорожное расстояние
            road_length += GetDistance(bus->stops[i], bus->stops[next_i]);
        }
    }
    // Для некольцевого маршрута
    else {
        // Прямой путь (туда)
        for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
            // Географическое расстояние
            geo_length += geo::ComputeDistance(bus->stops[i]->coordinates,
                                             bus->stops[i + 1]->coordinates);
            
            // Дорожное расстояние
            road_length += GetDistance(bus->stops[i], bus->stops[i + 1]);
        }
        
        // Обратный путь (обратно) - удваиваем географическое расстояние
        geo_length *= 2;
        
        // Для дорожного расстояния обратный путь может быть другим
        for (size_t i = bus->stops.size() - 1; i > 0; --i) {
            road_length += GetDistance(bus->stops[i], bus->stops[i - 1]);
        }
    }

    info.route_length = road_length;
    
    // Вычисляем извилистость
    if (geo_length > 0) {
        info.curvature = road_length / geo_length;
    } else {
        info.curvature = 0.0;
    }

    return info;
}

} // namespace transport_catalogue

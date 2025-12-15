#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <algorithm>
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

const Bus* TransportCatalogue::GetBus(std::string_view name_number) const {
    auto it = busname_to_bus_.find(name_number);
    return it != busname_to_bus_.end() ? it->second : nullptr;
}

const Stop* TransportCatalogue::GetStop(std::string_view name) const {
    auto it = stopname_to_stop_.find(name);
    return it != stopname_to_stop_.end() ? it->second : nullptr;
}

std::set<const Bus*, detail::BusPtrCompare> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
        return it->second;
    }
    return {};
}

std::optional<RouteInfo> TransportCatalogue::GetRouteInfo(std::string_view name) const {
    const Bus* bus = GetBus(name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }

    RouteInfo info{0, 0, 0.0, 0.0};
    info.stops_count = bus->stops.size();
    std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stops_count = unique_stops.size();

    double geo_length = 0.0;
    double real_length = 0.0;

    if (bus->is_roundtrip) {
        for (size_t i = 0; i < bus->stops.size(); ++i) {
            const Stop* from = bus->stops[i];
            const Stop* to = bus->stops[(i + 1) % bus->stops.size()];

            geo_length += ComputeDistance(from->coordinates, to->coordinates);
            
            int distance = GetDistance(from->name, to->name);
            if (distance != -1) {
                real_length += distance;
            } else {
                real_length += ComputeDistance(from->coordinates, to->coordinates);
            }
        }
    } else {
        for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
            const Stop* from = bus->stops[i];
            const Stop* to = bus->stops[i + 1];

            double segment_geo = ComputeDistance(from->coordinates, to->coordinates);
            geo_length += segment_geo;

            int distance = GetDistance(from->name, to->name);
            if (distance != -1) {
                real_length += distance;
            } else {
                real_length += segment_geo;
            }
        }
    }

    info.route_length = real_length;
    info.curvature = geo_length > 0 ? real_length / geo_length : 0.0;
    
    return info;
}

void TransportCatalogue::SetDistance(std::string_view from, std::string_view to, int meters) {
    distances_[{from, to}] = meters;
}

int TransportCatalogue::GetDistance(std::string_view from, std::string_view to) const {
    if (distances_.count({from, to})) {
        return distances_.at({from, to});
    }
    if (distances_.count({to, from})) {
        return distances_.at({to, from});
    }
    return -1;
}

} // namespace transport_catalogue

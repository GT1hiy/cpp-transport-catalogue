#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <cmath>
#include "geo.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string& name, geo::Coordinates coordinates) {
    all_stops_.push_back({name, coordinates});
    stopname_to_stop_[all_stops_.back().name] = &all_stops_.back();
    stop_to_buses_[all_stops_.back().name];
}

void TransportCatalogue::AddBus(const std::string& name_number, const std::vector<std::string>& stops, bool is_roundtrip) {
    std::vector<const domain::Stop*> bus_stops;
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
    auto it = busname_to_bus_.find(name_number);
    if (it == busname_to_bus_.end()) {
        return;
    }

    const domain::Bus* bus_ptr = it->second;

    for (const auto& stop_name : stops) {
        if (auto it = stopname_to_stop_.find(stop_name); it != stopname_to_stop_.end()) {
            stop_to_buses_[stop_name].insert(bus_ptr);
        }
    }
}

const domain::Bus* TransportCatalogue::GetBus(std::string_view name_number) const {
    auto it = busname_to_bus_.find(name_number);
    if (it != busname_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const domain::Stop* TransportCatalogue::GetStop(std::string_view name) const {
    auto it = stopname_to_stop_.find(name);
    if (it != stopname_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

std::set<const domain::Bus*, detail::BusPtrCompare> TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    if (auto it = stop_to_buses_.find(stop_name); it != stop_to_buses_.end()) {
        return it->second;
    }
    return {};
}

std::optional<domain::RouteInfo> TransportCatalogue::GetRouteInfo(std::string_view number_name) const {
    domain::RouteInfo info{0, 0, 0.0, 0.0};

    const domain::Bus* bus = GetBus(number_name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }

    std::unordered_set<const domain::Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stops_count = static_cast<int>(unique_stops.size());

    if (bus->is_roundtrip) {
        info.stops_count = static_cast<int>(bus->stops.size());
    } else {
        info.stops_count = static_cast<int>(2 * bus->stops.size() - 1);
    }

    double geo_length = 0.0;
    double real_length = 0.0;

    if (bus->is_roundtrip) {
        for (size_t i = 0; i < bus->stops.size(); ++i) {
            const domain::Stop* from = bus->stops[i];
            const domain::Stop* to = bus->stops[(i + 1) % bus->stops.size()];

            geo_length += geo::ComputeDistance(from->coordinates, to->coordinates);
            int distance = GetDistance(from, to);
            if (distance != 0) {
                real_length += distance;
            } else {
                real_length += geo::ComputeDistance(from->coordinates, to->coordinates);
            }
        }
    } else {
        // Прямое направление
        for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
            const domain::Stop* from = bus->stops[i];
            const domain::Stop* to = bus->stops[i + 1];
            geo_length += geo::ComputeDistance(from->coordinates, to->coordinates);
            real_length += GetDistance(from, to);
        }
        // Обратное направление
        for (size_t i = bus->stops.size() - 1; i > 0; --i) {
            const domain::Stop* from = bus->stops[i];
            const domain::Stop* to = bus->stops[i - 1];
            geo_length += geo::ComputeDistance(from->coordinates, to->coordinates);
            real_length += GetDistance(from, to);
        }
    }

    info.route_length = real_length;
    info.curvature = geo_length > 0 ? real_length / geo_length : 0.0;

    return info;
}

void TransportCatalogue::SetDistance(const domain::Stop* from, const domain::Stop* to, int meters) {
    std::pair<const domain::Stop*, const domain::Stop*> var = {from, to};
    distances_[var] = meters;
}

int TransportCatalogue::GetDistance(const domain::Stop* from_stop, const domain::Stop* to_stop) const {
    auto key_pair_from_to = std::make_pair(from_stop, to_stop);
    auto key_pair_to_from = std::make_pair(to_stop, from_stop);

    auto itf = distances_.find(key_pair_from_to);
    if (itf != distances_.end()) {
        return itf->second;
    }

    auto itt = distances_.find(key_pair_to_from);
    if (itt != distances_.end()) {
        return itt->second;
    }

    return 0;
}

void TransportCatalogue::AddDistance(const std::string& name, const std::vector<std::pair<int, std::string>>& pvc) {
    const domain::Stop* from_stop = GetStop(name);
    if (!from_stop) {
        return;
    }

    for (const auto& [distance, to_stop_name] : pvc) {
        const domain::Stop* to_stop = GetStop(to_stop_name);
        if (!to_stop) {
            continue;
        }
        SetDistance(from_stop, to_stop, distance);
    }
}

} // namespace transport_catalogue
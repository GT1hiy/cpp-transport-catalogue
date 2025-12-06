#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <deque>
#include <set>
#include <optional>
#include <utility>
#include "geo.h"

namespace transport_catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
};

struct RouteInfo {
    size_t stops_count;
    size_t unique_stops_count;
    double route_length;
    double curvature;
};

struct StopPairHasher {
    size_t operator()(const std::pair<const Stop*, const Stop*>& pair) const {
        auto hash1 = std::hash<const void*>{}(pair.first);
        auto hash2 = std::hash<const void*>{}(pair.second);
        return hash1 ^ (hash2 << 1);
    }
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, geo::Coordinates coordinates);
    void AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_roundtrip);
    
    // Методы для работы с расстояниями
    void SetDistance(const std::string& from, const std::string& to, int distance);
    int GetDistance(const Stop* from, const Stop* to) const;
    int GetDistance(const std::string& from, const std::string& to) const;

    const Bus* GetBus(std::string_view name_number) const;
    const Stop* GetStop(std::string_view name) const;

    std::set<const Bus*> GetBusesForStop(std::string_view stop_name) const;

    std::optional<RouteInfo> RouteInformation(std::string_view number_name) const;

private:
    void UpdateStopToBus(const std::string& name_number, const std::vector<std::string>& stops);

    std::deque<Bus> all_buses_;
    std::deque<Stop> all_stops_;

    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<std::string_view, std::set<const Bus*>> stop_to_buses_;
    
    // Хранилище расстояний между остановками
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHasher> distances_;
};

} // namespace transport_catalogue

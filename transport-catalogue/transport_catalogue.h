#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <deque>
#include <set>
#include "geo.h"

namespace transport_catalogue {

namespace detail {
    struct PairHasher {
        size_t operator()(const std::pair<std::string_view, std::string_view>& pr) const {
            auto h1 = std::hash<std::string_view>{}(pr.first);
            auto h2 = std::hash<std::string_view>{}(pr.second);
            return h1 + h2 * 37;
        }
    };

    struct BusPtrCompare {
        bool operator()(const Bus* lhs, const Bus* rhs) const {
            return lhs->name < rhs->name;
        }
    };
} // namespace detail

struct Stop {
    std::string name;
    Coordinates coordinates;
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

class TransportCatalogue {
public:
    void AddStop(const std::string& name, Coordinates coordinates);
    void AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_roundtrip);

    std::optional<RouteInfo> GetRouteInfo(std::string_view name) const;
    const Bus* GetBus(std::string_view name) const;
    const Stop* GetStop(std::string_view name) const;

    std::set<const Bus*, detail::BusPtrCompare> GetBusesForStop(std::string_view stop_name) const;

    void SetDistance(std::string_view from, std::string_view to, int meters);
    int GetDistance(std::string_view from, std::string_view to) const;

private:
    void UpdateStopToBus(const std::string& name_number, const std::vector<std::string>& stops);

    std::deque<Bus> all_buses_;
    std::deque<Stop> all_stops_;

    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<std::string_view, std::set<const Bus*, detail::BusPtrCompare>> stop_to_buses_;
    std::unordered_map<std::pair<std::string_view, std::string_view>, int, detail::PairHasher> distances_;
};

} // namespace transport_catalogue

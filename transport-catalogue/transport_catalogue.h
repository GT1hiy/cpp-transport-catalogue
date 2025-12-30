#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <deque>
#include <set>
#include "geo.h"
#include "domain.h"

namespace transport_catalogue {

using namespace geo;
using namespace domain;

namespace detail {
    struct PairHasher {
        size_t operator()(const std::pair<const Stop*, const Stop*> pr) const {
            auto h1 = std::hash<const Stop*>{}(pr.first);
            auto h2 = std::hash<const Stop*>{}(pr.second);
            return h1 + h2 * 37;
        }
    };

    struct BusPtrCompare {
        bool operator()(const Bus* lhs, const Bus* rhs) const {
            return lhs->name < rhs->name;
        }
    };
} // namespace detail

class TransportCatalogue {
public:
    void AddStop(const std::string& name, Coordinates coordinates);
    void AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_roundtrip);

    const Bus* GetBus(std::string_view name) const;
    const Stop* GetStop(std::string_view name) const;

    std::set<const Bus*, detail::BusPtrCompare> GetBusesForStop(std::string_view stop_name) const;
    std::optional<RouteInfo> GetRouteInfo(std::string_view name) const;

    //дистанция между остановками
    void AddDistance(const std::string& name, std::vector<std::pair<int, std::string>>& pvc);
    void SetDistance(const Stop* from, const Stop* to, int meters);
    int GetDistance(const Stop* from, const Stop* to) const;

    const std::unordered_map<std::string_view, const Bus*>& GetBusnameToBus() const {
        return busname_to_bus_;
    }

private:
    void UpdateStopToBus(const std::string& name_number, const std::vector<std::string>& stops);

    std::deque<Bus> all_buses_;
    std::deque<Stop> all_stops_;

    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
    std::unordered_map<std::string_view, std::set<const Bus*, detail::BusPtrCompare>> stop_to_buses_;
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, detail::PairHasher> distances_;
};

} // namespace transport_catalogue
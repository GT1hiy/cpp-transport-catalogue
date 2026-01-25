#pragma once 

#include <string> 
#include <string_view> 
#include <vector> 
#include <unordered_map> 
#include <optional> 
#include <deque> 
#include <set> 
#include <memory>
#include <chrono>
#include "geo.h" 
#include "domain.h" 

namespace transport_catalogue {

using Minutes = std::chrono::duration<double, std::chrono::minutes::period>;

struct RouteData {
    Minutes total_time;
    std::vector<std::variant<domain::WaitItem, domain::BusItem>> items;
};

namespace detail { 
    struct PairHasher { 
        size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*> pr) const { 
            auto h1 = std::hash<const domain::Stop*>{}(pr.first); 
            auto h2 = std::hash<const domain::Stop*>{}(pr.second); 
            return h1 + h2 * 37; 
        } 
    }; 

    struct BusPtrCompare { 
        bool operator()(const domain::Bus* lhs, const domain::Bus* rhs) const { 
            return lhs->name < rhs->name; 
        } 
    }; 
} // namespace detail 

class TransportRouter;

class TransportCatalogue { 
public: 
    TransportCatalogue();
    ~TransportCatalogue();
    
    // Запрещаем копирование и присваивание
    TransportCatalogue(const TransportCatalogue&) = delete;
    TransportCatalogue& operator=(const TransportCatalogue&) = delete;
    
    void AddStop(const std::string& name, geo::Coordinates coordinates); 
    void AddBus(const std::string& name, const std::vector<std::string>& stops, bool is_roundtrip); 

    const domain::Bus* GetBus(std::string_view name) const; 
    const domain::Stop* GetStop(std::string_view name) const; 

    std::set<const domain::Bus*, detail::BusPtrCompare> GetBusesForStop(std::string_view stop_name) const; 
    std::optional<domain::RouteInfo> GetRouteInfo(std::string_view name) const; 

    void AddDistance(const std::string& name, const std::vector<std::pair<int, std::string>>& pvc); 
    void SetDistance(const domain::Stop* from, const domain::Stop* to, int meters); 
    int GetDistance(const domain::Stop* from, const domain::Stop* to) const; 

    const std::unordered_map<std::string_view, const domain::Bus*>& GetBusnameToBus() const { 
        return busname_to_bus_; 
    }
    
    const std::unordered_map<std::string_view, const domain::Stop*>& GetStopnameToStop() const {
        return stopname_to_stop_;
    }
    
    // Маршрутизация
    void SetRouteSettings(const domain::RouteSettings& settings);
    std::optional<RouteData> BuildRoute(std::string_view from, std::string_view to) const;

private: 
    void UpdateStopToBus(const std::string& name_number, const std::vector<std::string>& stops); 

    std::deque<domain::Bus> all_buses_; 
    std::deque<domain::Stop> all_stops_; 

    std::unordered_map<std::string_view, const domain::Stop*> stopname_to_stop_; 
    std::unordered_map<std::string_view, const domain::Bus*> busname_to_bus_; 
    std::unordered_map<std::string_view, std::set<const domain::Bus*, detail::BusPtrCompare>> stop_to_buses_; 
    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, detail::PairHasher> distances_;
    
    class RouterImpl;
    RouterImpl* router_impl_;
}; 

} // namespace transport_catalogue
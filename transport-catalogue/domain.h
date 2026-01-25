#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <chrono>
#include "geo.h"

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip;
    
    Bus() = default;
    Bus(std::string n, std::vector<const Stop*> s, bool r)
        : name(std::move(n)), stops(std::move(s)), is_roundtrip(r) {}
};

struct RouteInfo {
    int stops_count = 0;
    int unique_stops_count = 0;
    double route_length = 0.0;
    double curvature = 0.0;
};

// Структуры для маршрутизации
struct RouteSettings {
    int bus_wait_time = 0;    // в минутах
    double bus_velocity = 0.0;  // в км/ч
};

// Элементы маршрута
struct WaitItem {
    std::string stop_name;
    double time;
};

struct BusItem {
    std::string bus;
    int span_count;
    double time;
};

} // namespace domain
#include "transport_router.h"
#include <cmath>
#include <algorithm>

namespace transport_catalogue {

TransportRouter::TransportRouter(const TransportCatalogue& catalogue, 
                               const domain::RouteSettings& settings)
    : catalogue_(catalogue)
    , settings_(settings) {
    BuildGraph();
}

void TransportRouter::BuildGraph() {
    // Очищаем предыдущие данные
    stop_to_vertex_.clear();
    edge_info_.clear();
    
    // 1. Получаем все остановки
    const auto& stops = catalogue_.GetStopnameToStop();
    if (stops.empty()) {
        graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(0);
        router_ = std::make_unique<graph::Router<double>>(*graph_);
        return;
    }
    
    // 2. Создаем вершины для каждой остановки (2 вершины на остановку)
    size_t vertex_count = stops.size() * 2;
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);
    
    // 3. Сопоставляем остановкам вершины
    graph::VertexId vertex_id = 0;
    for (const auto& [name, stop] : stops) {
        stop_to_vertex_[stop] = vertex_id;
        vertex_id += 2; // 2 вершины на остановку
    }
    
    // 4. Добавляем рёбра ожидания (от вершины ожидания к вершине посадки)
    for (const auto& [stop_ptr, wait_vertex] : stop_to_vertex_) {
        graph::Edge<double> wait_edge{
            wait_vertex,                    // from - вершина ожидания
            wait_vertex + 1,                // to - вершина посадки
            static_cast<double>(settings_.bus_wait_time) // вес - время ожидания
        };
        
        auto edge_id = graph_->AddEdge(wait_edge);
        edge_info_[edge_id] = {wait_vertex, wait_vertex + 1, 
                              static_cast<double>(settings_.bus_wait_time),
                              nullptr, 0, true};
    }
    
    // 5. Добавляем рёбра поездки на автобусах
    for (const auto& [bus_name, bus] : catalogue_.GetBusnameToBus()) {
        AddBusEdgesForRoute(bus);
    }
    
    // 6. Создаем роутер
    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

void TransportRouter::AddBusEdgesForRoute(const domain::Bus* bus) {
    if (!bus || bus->stops.size() < 2) {
        return;
    }
    
    // Для каждой пары остановок на маршруте (i -> j, где j > i)
    for (size_t i = 0; i < bus->stops.size(); ++i) {
        for (size_t j = i + 1; j < bus->stops.size(); ++j) {
            // Вычисляем общее расстояние между остановками i и j
            double total_distance = 0.0;
            bool valid = true;
            
            for (size_t k = i; k < j; ++k) {
                const auto* from_stop = bus->stops[k];
                const auto* to_stop = bus->stops[k + 1];
                
                // Пробуем получить расстояние в прямом направлении
                int dist = catalogue_.GetDistance(from_stop, to_stop);
                if (dist == 0) {
                    // Если нет расстояния, этот сегмент невалиден
                    valid = false;
                    break;
                }
                total_distance += dist;
            }
            
            if (!valid || total_distance <= 0) {
                continue;
            }
            
            // Вычисляем время поездки в минутах
            // скорость в км/ч * 1000 / 60 = скорость в метрах в минуту
            double speed_m_per_min = settings_.bus_velocity * 1000.0 / 60.0;
            double time = total_distance / speed_m_per_min;
            int span_count = static_cast<int>(j - i);
            
            // Добавляем ребро от i к j
            const auto* from_stop = bus->stops[i];
            const auto* to_stop = bus->stops[j];
            
            graph::Edge<double> bus_edge{
                stop_to_vertex_.at(from_stop) + 1,  // from - вершина посадки
                stop_to_vertex_.at(to_stop),        // to - вершина ожидания
                time
            };
            
            auto edge_id = graph_->AddEdge(bus_edge);
            edge_info_[edge_id] = {stop_to_vertex_.at(from_stop) + 1,
                                  stop_to_vertex_.at(to_stop),
                                  time,
                                  bus,
                                  span_count,
                                  false};
            
            // Для кольцевого маршрута, если это последний сегмент, добавляем ребро замыкания
            if (bus->is_roundtrip && i == 0 && j == bus->stops.size() - 1) {
                // Добавляем ребро от последней остановки к первой
                double circle_distance = 0.0;
                valid = true;
                
                // Вычисляем расстояние от последней к первой
                const auto* last_stop = bus->stops.back();
                const auto* first_stop = bus->stops.front();
                int dist = catalogue_.GetDistance(last_stop, first_stop);
                if (dist == 0) {
                    valid = false;
                } else {
                    circle_distance = dist;
                }
                
                if (valid && circle_distance > 0) {
                    double circle_time = circle_distance / speed_m_per_min;
                    
                    graph::Edge<double> circle_edge{
                        stop_to_vertex_.at(last_stop) + 1,
                        stop_to_vertex_.at(first_stop),
                        circle_time
                    };
                    
                    auto circle_edge_id = graph_->AddEdge(circle_edge);
                    edge_info_[circle_edge_id] = {stop_to_vertex_.at(last_stop) + 1,
                                                 stop_to_vertex_.at(first_stop),
                                                 circle_time,
                                                 bus,
                                                 1,
                                                 false};
                }
            }
        }
    }
    
    // Для некольцевого маршрута добавляем обратные рёбра
    if (!bus->is_roundtrip) {
        // Обратное направление (от конечной к начальной)
        for (size_t i = bus->stops.size() - 1; i > 0; --i) {
            for (size_t j = i - 1; j != static_cast<size_t>(-1); --j) {
                // Вычисляем расстояние в обратном направлении
                double total_distance = 0.0;
                bool valid = true;
                
                for (size_t k = i; k > j; --k) {
                    const auto* from_stop = bus->stops[k];
                    const auto* to_stop = bus->stops[k - 1];
                    
                    int dist = catalogue_.GetDistance(from_stop, to_stop);
                    if (dist == 0) {
                        valid = false;
                        break;
                    }
                    total_distance += dist;
                }
                
                if (!valid || total_distance <= 0) {
                    continue;
                }
                
                // Вычисляем время поездки
                double speed_m_per_min = settings_.bus_velocity * 1000.0 / 60.0;
                double time = total_distance / speed_m_per_min;
                int span_count = static_cast<int>(i - j);
                
                // Добавляем ребро
                const auto* from_stop = bus->stops[i];
                const auto* to_stop = bus->stops[j];
                
                graph::Edge<double> bus_edge{
                    stop_to_vertex_.at(from_stop) + 1,
                    stop_to_vertex_.at(to_stop),
                    time
                };
                
                auto edge_id = graph_->AddEdge(bus_edge);
                edge_info_[edge_id] = {stop_to_vertex_.at(from_stop) + 1,
                                      stop_to_vertex_.at(to_stop),
                                      time,
                                      bus,
                                      span_count,
                                      false};
            }
        }
    }
}

std::optional<RouteData> TransportRouter::BuildRoute(std::string_view from, 
                                                     std::string_view to) const {
    RouteData result;
    
    auto from_stop = catalogue_.GetStop(from);
    auto to_stop = catalogue_.GetStop(to);
    
    if (!from_stop || !to_stop || !router_) {
        return std::nullopt;
    }
    
    auto from_vertex = stop_to_vertex_.at(from_stop);
    auto to_vertex = stop_to_vertex_.at(to_stop);
    
    auto route_info = router_->BuildRoute(from_vertex, to_vertex);
    
    if (!route_info) {
        return std::nullopt;
    }
    
    result.total_time = Minutes(route_info->weight);
    
    for (auto edge_id : route_info->edges) {
        const auto& edge = edge_info_.at(edge_id);
        
        if (edge.is_wait) {
            // Это ребро ожидания
            const domain::Stop* stop = nullptr;
            for (const auto& [s, v] : stop_to_vertex_) {
                if (v == edge.from) {
                    stop = s;
                    break;
                }
            }
            
            if (stop) {
                domain::WaitItem wait_item{
                    stop->name,
                    edge.weight
                };
                result.items.push_back(wait_item);
            }
        } else {
            // Это ребро поездки на автобусе
            if (edge.bus_ptr) {
                domain::BusItem bus_item{
                    edge.bus_ptr->name,
                    edge.span_count,
                    edge.weight
                };
                result.items.push_back(bus_item);
            }
        }
    }
    
    return result;
}

} // namespace transport_catalogue
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <optional>
#include "domain.h"
#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport_catalogue {

using Minutes = std::chrono::duration<double, std::chrono::minutes::period>;

struct RouteData {
    Minutes total_time;
    std::vector<std::variant<domain::WaitItem, domain::BusItem>> items;
};

class TransportRouter {
public:
    explicit TransportRouter(const TransportCatalogue& catalogue, 
                            const domain::RouteSettings& settings);
    
    std::optional<RouteData> BuildRoute(std::string_view from, std::string_view to) const;
    
private:
    struct ExtendedEdge {
        graph::VertexId from;
        graph::VertexId to;
        double weight;
        const domain::Bus* bus_ptr = nullptr;
        int span_count = 0;
        bool is_wait = false;
    };
    
    void BuildGraph();
    void AddBusEdgesForRoute(const domain::Bus* bus);
    
    const TransportCatalogue& catalogue_;
    domain::RouteSettings settings_;
    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::unordered_map<const domain::Stop*, graph::VertexId> stop_to_vertex_;
    std::unordered_map<graph::EdgeId, ExtendedEdge> edge_info_;
};

} // namespace transport_catalogue
#pragma once

#include <istream>
#include <string>
#include <vector>
#include <map>

#include "json.h"
#include "domain.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

namespace request_handler {
    class RequestHandler;
}

namespace json_reader {

class JsonReader {
public:
    JsonReader(std::istream& input, 
               transport_catalogue::TransportCatalogue& catalogue,
               renderer::MapRenderer& render);
    
    const json::Document& GetDocument() const;
    json::Node LoadDataFromJson();
    
    json::Document HandleJsonRequest(const json::Node& json_request,
                                     request_handler::RequestHandler& request_handler);
    
    void HandRenderSettings();
    
    const json::Node& GetRenderSettings() const;
    const json::Node& GetStatRequests() const;
    const json::Node& GetRoutingSettings() const;
    
private:
    json::Node ProcessBusRequest(const json::Dict& request, int id) const;
    json::Node ProcessStopRequest(const json::Dict& request, int id) const;
    json::Node ProcessMapRequest(int id,
                                 request_handler::RequestHandler& request_handler) const;
    json::Node ProcessRouteRequest(const json::Dict& request, int id) const;
    
    void ParseBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const;
    void ParseStops(transport_catalogue::TransportCatalogue& catalogue, 
                    const json::Array& base_requests) const;
    void ParseBuses(transport_catalogue::TransportCatalogue& catalogue, 
                    const json::Array& base_requests) const;
    void ParseDistances(transport_catalogue::TransportCatalogue& catalogue, 
                        const json::Array& base_requests) const;
    
    renderer::RenderSettings ParseRenderSettings(const json::Node& root) const;
    domain::RouteSettings ParseRoutingSettings(const json::Node& root) const;
    svg::Color ParseColor(const json::Node& color_node) const;
    
    json::Document doc_input_;
    transport_catalogue::TransportCatalogue& catalogue_;
    renderer::MapRenderer& render_;
    json::Node null_node_{nullptr};
};

} // namespace json_reader
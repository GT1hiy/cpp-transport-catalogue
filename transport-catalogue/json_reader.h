#pragma once

#include <istream>
#include <memory>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

namespace json_reader {

class JsonReader {
public:
    explicit JsonReader(std::istream& input, 
                       transport_catalogue::TransportCatalogue& catalogue, 
                       renderer::MapRenderer& render);

    const json::Document& GetDocument() const;
    
    json::Node LoadDataFromJson();
    json::Document HandleJsonRequest(const json::Node& json_request);
    void HandRenderSettings();

private:
    const json::Node& GetRenderSettings() const;
    const json::Node& GetStatRequests() const;
    
    svg::Color ParseColor(const json::Node& color_node) const;
    renderer::RenderSettings ParseRenderSettings(const json::Node& root) const;
    
    void ParseBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const;
    void ParseStops(transport_catalogue::TransportCatalogue& catalogue, 
                   const json::Array& base_requests) const;
    void ParseBuses(transport_catalogue::TransportCatalogue& catalogue, 
                   const json::Array& base_requests) const;
    void ParseDistances(transport_catalogue::TransportCatalogue& catalogue, 
                       const json::Array& base_requests) const;

    json::Document doc_input_;
    json::Node null_node_ = nullptr;
    transport_catalogue::TransportCatalogue& catalogue_;
    renderer::MapRenderer& render_;
};

} // namespace json_reader

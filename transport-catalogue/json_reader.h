#pragma once

#include "request_handler.h"
#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

namespace json {
    class Document;
    class Node;
}

namespace request_handler {
    class RequestHandler;
}

namespace json_reader {

    class JsonReader {
    public:
        explicit JsonReader(std::istream& input, transport_catalogue::TransportCatalogue& catalogue, renderer::MapRenderer& render);

        const json::Document& GetDocument() const;
        const json::Node& GetRenderSettings() const;
        const json::Node& GetStatRequests() const;

        json::Node LoadDataFromJson();
        json::Document HandleJsonRequest(const json::Node& json_request, request_handler::RequestHandler& request_handler);
        void HandRenderSettings();

        renderer::RenderSettings ParseRenderSettings(const json::Node& root) const;

    private:
        // Методы обработки отдельных запросов
        json::Node ProcessBusRequest(const json::Dict& request) const;
        json::Node ProcessStopRequest(const json::Dict& request) const;
        json::Node ProcessMapRequest(const json::Dict& request, request_handler::RequestHandler& request_handler) const;

        // Вспомогательные методы
        svg::Color ParseColor(const json::Node& color_node) const;
        void ParseBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const;
        void ParseStops(transport_catalogue::TransportCatalogue& catalogue, const json::Array& base_requests) const;
        void ParseBuses(transport_catalogue::TransportCatalogue& catalogue, const json::Array& base_requests) const;
        void ParseDistances(transport_catalogue::TransportCatalogue& catalogue, const json::Array& base_requests) const;

        // Хранилище распарсенных данных
        json::Document doc_input_;
        json::Node null_node_ = nullptr;
        transport_catalogue::TransportCatalogue& catalogue_;
        renderer::MapRenderer& render_;
    };

} // namespace json_reader
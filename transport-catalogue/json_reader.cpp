#include <istream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "domain.h"
#include "json.h"
#include "json_reader.h"
#include "json_builder.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace json_reader {

using namespace domain;
using namespace json;
using namespace std::string_literals;
using namespace std;

JsonReader::JsonReader(std::istream& input, 
                       transport_catalogue::TransportCatalogue& catalogue,
                       renderer::MapRenderer& render)
    : doc_input_(json::Load(input))
    , catalogue_(catalogue)
    , render_(render) {
}

const json::Document& JsonReader::GetDocument() const {
    return doc_input_;
}

json::Node JsonReader::LoadDataFromJson() {
    ParseBaseRequests(catalogue_);
    
    if (auto render_settings = GetRenderSettings(); render_settings != nullptr) {
        render_.SetRenderSettings(ParseRenderSettings(render_settings));
    }
    
    return GetStatRequests();
}

json::Document JsonReader::HandleJsonRequest(const json::Node& json_request,
                                             request_handler::RequestHandler& request_handler) {
    using namespace json;
    using namespace std;

    const Node& root = json_request;

    if (!root.IsArray()) {
        throw invalid_argument("Invalid JSON format: stat_requests should be an array"s);
    }

    Builder builder;
    auto array_context = builder.StartArray();
    
    const Array& requests = root.AsArray();

    for (const Node& request_node : requests) {
        const Dict& request = request_node.AsMap();
        int id = request.at("id"s).AsInt();
        string type = request.at("type"s).AsString();

        try {
            if (type == "Bus"s) {
                Node response = ProcessBusRequest(request, id);
                array_context.Value(response.GetValue());
            } else if (type == "Stop"s) {
                Node response = ProcessStopRequest(request, id);
                array_context.Value(response.GetValue());
            } else if (type == "Map"s) {
                Node response = ProcessMapRequest(id, request_handler);
                array_context.Value(response.GetValue());
            } else {
                Builder error_builder;
                error_builder.StartDict()
                           .Key("request_id"s).Value(id)
                           .Key("error_message"s).Value("unknown request type: "s + type)
                           .EndDict();
                array_context.Value(error_builder.Build().GetValue());
            }
        } catch (const exception& e) {
            Builder error_builder;
            error_builder.StartDict()
                       .Key("request_id"s).Value(id)
                       .Key("error_message"s).Value(string(e.what()))
                       .EndDict();
            array_context.Value(error_builder.Build().GetValue());
        }
    }

    array_context.EndArray();
    return json::Document(builder.Build());
}

json::Node JsonReader::ProcessBusRequest(const json::Dict& request, int id) const {
    Builder builder;
    
    string name = request.at("name"s).AsString();
    const Bus* bus = catalogue_.GetBus(name);

    if (!bus) {
        builder.StartDict()
               .Key("request_id"s).Value(id)
               .Key("error_message"s).Value("not found"s)
               .EndDict();
    } else {
        auto route_info_opt = catalogue_.GetRouteInfo(name);
        if (route_info_opt) {
            auto& info = *route_info_opt;
            builder.StartDict()
                   .Key("request_id"s).Value(id)
                   .Key("route_length").Value(static_cast<int>(info.route_length))
                   .Key("curvature").Value(info.curvature)
                   .Key("stop_count").Value(static_cast<int>(info.stops_count))
                   .Key("unique_stop_count").Value(static_cast<int>(info.unique_stops_count))
                   .EndDict();
        } else {
            builder.StartDict()
                   .Key("request_id"s).Value(id)
                   .Key("error_message"s).Value("not found"s)
                   .EndDict();
        }
    }
    
    return builder.Build();
}

json::Node JsonReader::ProcessStopRequest(const json::Dict& request, int id) const {
    Builder builder;
    
    string name = request.at("name"s).AsString();
    const Stop* stop = catalogue_.GetStop(name);

    if (!stop) {
        builder.StartDict()
               .Key("request_id"s).Value(id)
               .Key("error_message"s).Value("not found"s)
               .EndDict();
    } else {
        auto buses = catalogue_.GetBusesForStop(name);
        
        builder.StartDict()
               .Key("request_id"s).Value(id)
               .Key("buses"s).StartArray();
        
        for (const Bus* bus : buses) {
            builder.Value(bus->name);
        }
        
        builder.EndArray()
               .EndDict();
    }
    
    return builder.Build();
}

json::Node JsonReader::ProcessMapRequest(int id,
                                         request_handler::RequestHandler& request_handler) const {
    Builder builder;
    
    std::ostringstream out;
    request_handler.RenderMap().Render(out);
    
    builder.StartDict()
           .Key("request_id"s).Value(id)
           .Key("map"s).Value(out.str())
           .EndDict();
    
    return builder.Build();
}

void JsonReader::HandRenderSettings() {
    std::ostringstream out_map;
    const json::Node rnd_sttng = GetRenderSettings();

    if (rnd_sttng != nullptr) {
        const json::Dict& render_settings_dict = rnd_sttng.AsMap();
        renderer::RenderSettings render_var = ParseRenderSettings(render_settings_dict);
        render_.SetRenderSettings(std::move(render_var));
    }
}

const json::Node& JsonReader::GetRenderSettings() const {
    if (!doc_input_.GetRoot().AsMap().count("render_settings"s)) {
        return null_node_;
    }
    return doc_input_.GetRoot().AsMap().at("render_settings"s);
}

const json::Node& JsonReader::GetStatRequests() const {
    if (!doc_input_.GetRoot().AsMap().count("stat_requests"s)) {
        return null_node_;
    }
    return doc_input_.GetRoot().AsMap().at("stat_requests"s);
}

void JsonReader::ParseBaseRequests(transport_catalogue::TransportCatalogue& catalogue) const {
    const json::Node& root = doc_input_.GetRoot();
    if (!root.IsMap()) return;

    const json::Dict& root_dict = root.AsMap();
    if (!root_dict.count("base_requests"s)) return;

    const json::Array& base_requests = root_dict.at("base_requests"s).AsArray();

    ParseStops(catalogue, base_requests);
    ParseBuses(catalogue, base_requests);
    ParseDistances(catalogue, base_requests);
}

void JsonReader::ParseBuses(transport_catalogue::TransportCatalogue& catalogue, 
                            const json::Array& base_requests) const {
    for (const json::Node& request_node : base_requests) {
        const json::Dict& request = request_node.AsMap();
        if (request.at("type"s).AsString() == "Bus"s) {
            const std::string& name = request.at("name"s).AsString();
            const json::Array& stops_array = request.at("stops"s).AsArray();
            std::vector<std::string> stops;
            stops.reserve(stops_array.size());

            for (const json::Node& stop_node : stops_array) {
                stops.push_back(stop_node.AsString());
            }

            bool is_roundtrip = request.at("is_roundtrip"s).AsBool();
            catalogue.AddBus(name, std::move(stops), is_roundtrip);
        }
    }
}

void JsonReader::ParseDistances(transport_catalogue::TransportCatalogue& catalogue, 
                                const json::Array& base_requests) const {
    for (const json::Node& request_node : base_requests) {
        const json::Dict& request = request_node.AsMap();
        if (request.at("type"s).AsString() == "Stop"s) {
            const std::string& from_name = request.at("name"s).AsString();

            if (request.count("road_distances"s)) {
                const json::Dict& distances = request.at("road_distances"s).AsMap();
                for (const auto& [to_name, dist_node] : distances) {
                    const Stop* from_stop = catalogue.GetStop(from_name);
                    const Stop* to_stop = catalogue.GetStop(to_name);
                    if (from_stop && to_stop) {
                        catalogue.SetDistance(from_stop, to_stop, dist_node.AsInt());
                    }
                }
            }
        }
    }
}

void JsonReader::ParseStops(transport_catalogue::TransportCatalogue& catalogue, 
                            const json::Array& base_requests) const {
    for (const json::Node& request_node : base_requests) {
        const json::Dict& request = request_node.AsMap();
        if (request.at("type"s).AsString() == "Stop"s) {
            const std::string& name = request.at("name"s).AsString();
            double lat = request.at("latitude"s).AsDouble();
            double lng = request.at("longitude"s).AsDouble();
            geo::Coordinates coords{lat, lng};
            catalogue.AddStop(name, coords);
        }
    }
}

svg::Color JsonReader::ParseColor(const json::Node& color_node) const {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const json::Array& color_array = color_node.AsArray();
        if (color_array.size() == 3) {
            return svg::Rgb(
                color_array[0].AsInt(),
                color_array[1].AsInt(),
                color_array[2].AsInt()
            );
        } else if (color_array.size() == 4) {
            return svg::Rgba(
                color_array[0].AsInt(),
                color_array[1].AsInt(),
                color_array[2].AsInt(),
                color_array[3].AsDouble()
            );
        } else {
            throw std::logic_error("Invalid color array size"s);
        }
    } else {
        throw std::logic_error("Invalid color type"s);
    }
}

renderer::RenderSettings JsonReader::ParseRenderSettings(const Node& root) const {
    renderer::RenderSettings render_settings;
    const json::Dict& request_map = root.AsMap();

    render_settings.width = request_map.at("width"s).AsDouble();
    render_settings.height = request_map.at("height"s).AsDouble();
    render_settings.padding = request_map.at("padding"s).AsDouble();
    render_settings.stop_radius = request_map.at("stop_radius"s).AsDouble();
    render_settings.line_width = request_map.at("line_width"s).AsDouble();
    render_settings.bus_label_font_size = request_map.at("bus_label_font_size"s).AsInt();
    const json::Array& bus_label_offset = request_map.at("bus_label_offset"s).AsArray();
    render_settings.bus_label_offset = {bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble()};
    render_settings.stop_label_font_size = request_map.at("stop_label_font_size"s).AsInt();
    const json::Array& stop_label_offset = request_map.at("stop_label_offset"s).AsArray();
    render_settings.stop_label_offset = {stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble()};

    render_settings.underlayer_color = ParseColor(request_map.at("underlayer_color"s));
    render_settings.underlayer_width = request_map.at("underlayer_width"s).AsDouble();

    const json::Array& color_palette = request_map.at("color_palette"s).AsArray();
    for (const auto& color_element : color_palette) {
        render_settings.color_palette.push_back(ParseColor(color_element));
    }

    return render_settings;
}

} // namespace json_reader
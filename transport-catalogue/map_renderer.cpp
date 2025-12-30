#include "map_renderer.h"
#include <vector>
#include <map>
#include <string_view>
#include <algorithm>

namespace renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

std::vector<svg::Polyline> MapRenderer::GetRouteLines(
    const std::map<std::string_view, const domain::Bus*>& buses,
    const SphereProjector& sp) const {
    std::vector<svg::Polyline> result;
    size_t color_num = 0;
    
    for (const auto& [bus_number, bus] : buses) {
        if (bus->stops.empty()) continue;
        
        std::vector<const domain::Stop*> route_stops{
            bus->stops.begin(), bus->stops.end()
        };
        
        if (!bus->is_roundtrip) {
            route_stops.insert(route_stops.end(),
                               std::next(bus->stops.rbegin()),
                               bus->stops.rend());
        }
        
        svg::Polyline line;
        for (const auto& stop : route_stops) {
            line.AddPoint(sp(stop->coordinates));
        }
        
        line.SetStrokeColor(render_settings_.color_palette[color_num])
            .SetFillColor("none")
            .SetStrokeWidth(render_settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        
        if (color_num < (render_settings_.color_palette.size() - 1)) {
            ++color_num;
        } else {
            color_num = 0;
        }
        
        result.push_back(std::move(line));
    }
    
    return result;
}

std::vector<svg::Text> MapRenderer::GetNameBusRoute(
    const std::map<std::string_view, const domain::Bus*>& buses,
    const SphereProjector& sp) const {  // Исправлено: добавлен const &
    
    std::vector<svg::Text> result;
    size_t color_num = 0;
    
    std::vector<std::pair<std::string_view, const domain::Bus*>> sorted_buses;
    sorted_buses.reserve(buses.size());
    
    for (const auto& [bus_name, bus] : buses) {
        sorted_buses.emplace_back(bus_name, bus);
    }
    
    std::sort(sorted_buses.begin(), sorted_buses.end(),
              [](const auto& lhs, const auto& rhs) {
                  return lhs.first < rhs.first;
              });
    
    for (const auto& [bus_name, bus] : sorted_buses) {
        if (bus->stops.empty()) {
            continue;
        }
        
        svg::Color bus_color = render_settings_.color_palette[color_num];
        
        std::vector<const domain::Stop*> end_stops;
        
        if (bus->is_roundtrip) {
            end_stops.push_back(bus->stops.front());
        } else {
            end_stops.push_back(bus->stops.front());
            if (bus->stops.front() != bus->stops.back()) {
                end_stops.push_back(bus->stops.back());
            }
        }
        
        for (const auto& stop : end_stops) {
            svg::Point stop_point = sp(stop->coordinates);
            
            svg::Text underlayer;
            underlayer.SetPosition(stop_point)
                .SetOffset(render_settings_.bus_label_offset)
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(std::string(bus_name))
                .SetFillColor(render_settings_.underlayer_color)
                .SetStrokeColor(render_settings_.underlayer_color)
                .SetStrokeWidth(render_settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            
            svg::Text label;
            label.SetPosition(stop_point)
                .SetOffset(render_settings_.bus_label_offset)
                .SetFontSize(render_settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(std::string(bus_name))
                .SetFillColor(bus_color);
            
            result.push_back(std::move(underlayer));
            result.push_back(std::move(label));
        }
        
        if (color_num < (render_settings_.color_palette.size() - 1)) {
            ++color_num;
        } else {
            color_num = 0;
        }
    }
    
    return result;
}

std::vector<svg::Circle> MapRenderer::GetStopsSymbols(
    const std::map<std::string_view, const domain::Stop*>& stops,
    const SphereProjector& sp) const {
    
    std::vector<svg::Circle> result;
    
    for (const auto& [stop_name, stop] : stops) {
        svg::Point stop_point = sp(stop->coordinates);
        
        svg::Circle circle;
        circle.SetCenter(stop_point)
            .SetRadius(render_settings_.stop_radius)
            .SetFillColor("white");
        
        result.push_back(std::move(circle));
    }
    
    return result;
}

std::vector<svg::Text> MapRenderer::GetStopsLabels(
    const std::map<std::string_view, const domain::Stop*>& stops,
    const SphereProjector& sp) const {
    
    std::vector<svg::Text> result;
    
    std::vector<std::pair<std::string_view, const domain::Stop*>> sorted_stops;
    sorted_stops.reserve(stops.size());
    
    for (const auto& [stop_name, stop] : stops) {
        if (!stop->name.empty()) {
            sorted_stops.emplace_back(stop_name, stop);
        }
    }
    
    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const auto& lhs, const auto& rhs) {
                  return lhs.first < rhs.first;
              });
    
    for (const auto& [stop_name, stop] : sorted_stops) {
        svg::Point stop_point = sp(stop->coordinates);
        
        svg::Text underlayer;
        underlayer.SetPosition(stop_point)
            .SetOffset(render_settings_.stop_label_offset)
            .SetFontSize(render_settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(std::string(stop_name))
            .SetFillColor(render_settings_.underlayer_color)
            .SetStrokeColor(render_settings_.underlayer_color)
            .SetStrokeWidth(render_settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        
        svg::Text label;
        label.SetPosition(stop_point)
            .SetOffset(render_settings_.stop_label_offset)
            .SetFontSize(render_settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(std::string(stop_name))
            .SetFillColor("black");
        
        result.push_back(std::move(underlayer));
        result.push_back(std::move(label));
    }
    
    return result;
}

svg::Document MapRenderer::GetSVG(
    const std::map<std::string_view, const domain::Bus*>& buses) const {
    
    svg::Document result;
    std::vector<geo::Coordinates> route_stops_coord;
    std::map<std::string_view, const domain::Stop*> all_stops;
    
    for (const auto& [bus_number, bus] : buses) {
        for (const auto& stop : bus->stops) {
            route_stops_coord.push_back(stop->coordinates);
            all_stops[stop->name] = stop;
        }
    }
    
    SphereProjector sp(route_stops_coord.begin(),
                       route_stops_coord.end(),
                       render_settings_.width,
                       render_settings_.height,
                       render_settings_.padding);
    
    for (auto&& line : GetRouteLines(buses, sp)) {
        result.Add(std::move(line));
    }
    
    for (auto&& text : GetNameBusRoute(buses, sp)) {
        result.Add(std::move(text));
    }
    
    for (auto&& circle : GetStopsSymbols(all_stops, sp)) {
        result.Add(std::move(circle));
    }
    
    for (auto&& text : GetStopsLabels(all_stops, sp)) {
        result.Add(std::move(text));
    }
    
    return result;
}

} // namespace renderer
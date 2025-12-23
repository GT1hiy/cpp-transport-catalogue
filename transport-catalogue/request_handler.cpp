#include "request_handler.h"
#include "json_reader.h"

namespace request_handler {

RequestHandler::RequestHandler (transport_catalogue::TransportCatalogue& catalogue,  renderer::MapRenderer& render):
    catalogue_(catalogue),
    render_(render)
{
}

svg::Document RequestHandler::RenderMap() const {
    std::map<std::string_view, const domain::Bus*> result;
    const auto& busname_to_bus_ = catalogue_.GetBusnameToBus();

    for (const auto& bus : busname_to_bus_) {
        if (bus.second != nullptr) {
            result.emplace(bus.first, bus.second);
        }
    }

    return render_.GetSVG(result);
}

std::optional<domain::Stop> RequestHandler::GetStopInfo(const std::string& stop_name) const {
    const auto* stop = catalogue_.GetStop(stop_name);
    if (!stop) {
        return std::nullopt;
    }
    return *stop;
}

std::optional<domain::Bus> RequestHandler::GetBusInfo(const std::string& bus_name) const {
    const auto* bus = catalogue_.GetBus(bus_name);
    if (!bus) {
        return std::nullopt;
    }
    return *bus;
}

std::optional<domain::RouteInfo> RequestHandler::GetRouteBetweenStops(
    const std::string& from_stop,
    const std::string& to_stop
    ) const {
    // Заглушка - нужно реализовать логику маршрутизации
    return std::nullopt;
}

json::Node RequestHandler::LoadDataFromJson() {
    // Заглушка - должен делегировать JsonReader
    return json::Node();
}

void RequestHandler::HandRenderSettings() {
    // Заглушка
}

json::Node RequestHandler::HandStatRequests() {
    // Заглушка
    return json::Node();
}

}// namespace request_handler

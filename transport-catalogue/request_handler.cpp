#include "request_handler.h"
#include "json_reader.h"

namespace request_handler {

RequestHandler::RequestHandler(transport_catalogue::TransportCatalogue& catalogue,  
                               renderer::MapRenderer& render):
    catalogue_(catalogue),
    render_(render)
{
}

svg::Document RequestHandler::RenderMap() const {
    std::map<std::string_view, const domain::Bus*> result;

    const auto& busname_to_bus = catalogue_.GetBusnameToBus();

    for (const auto& bus : busname_to_bus) {
        if (bus.second != nullptr) {
            result.emplace(bus.first, bus.second);
        }
    }

    return render_.GetSVG(result);
}

} // namespace request_handler
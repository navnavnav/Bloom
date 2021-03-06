#include "DiscoveryResponseFrame.hpp"

#include "src/Exceptions/Exception.hpp"

namespace Bloom::DebugToolDrivers::Protocols::CmsisDap::Edbg::Avr::ResponseFrames::Discovery
{
    using namespace Bloom::Exceptions;

    ResponseId DiscoveryResponseFrame::getResponseId() {
        const auto& payload = this->getPayload();

        if (payload.empty()) {
            throw Exception("Response ID missing from DISCOVERY response frame payload.");
        }

        return static_cast<ResponseId>(payload[0]);
    }

    std::vector<unsigned char> DiscoveryResponseFrame::getPayloadData() {
        const auto& payload = this->getPayload();

        // DISCOVERY payloads include two bytes before the data (response ID and version byte).
        auto data = std::vector<unsigned char>(
            payload.begin() + 2,
            payload.end()
        );

        return data;
    }
}

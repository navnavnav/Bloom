#pragma once

#include <src/DebugToolDrivers/Protocols/CMSIS-DAP/VendorSpecific/EDBG/AVR/ResponseFrames/DiscoveryResponseFrame.hpp>
#include "../AvrCommandFrame.hpp"

namespace Bloom::DebugToolDrivers::Protocols::CmsisDap::Edbg::Avr::CommandFrames::Discovery
{
    using namespace DebugToolDrivers::Protocols::CmsisDap;
    /**
     * Discovery commands can only return two responses; A LIST response and a failure.
     */
    enum class ResponseId : unsigned char
    {
        /*
         * According to the protocol docs, response ID 0x81 is for a LIST response, but this doesn't seem to be
         * well defined. So just going to use a generic name.
         */
        OK = 0x81,
        FAILED = 0xA0,
    };

    inline bool operator==(unsigned char rawId, ResponseId id) {
        return static_cast<unsigned char>(id) == rawId;
    }

    inline bool operator==(ResponseId id, unsigned char rawId) {
        return static_cast<unsigned char>(id) == rawId;
    }

    inline bool operator!=(unsigned char rawId, ResponseId id) {
        return static_cast<unsigned char>(id) != rawId;
    }

    inline bool operator!=(ResponseId id, unsigned char rawId) {
        return static_cast<unsigned char>(id) != rawId;
    }

    class DiscoveryCommandFrame: public AvrCommandFrame
    {
    public:
        using ResponseFrameType = ResponseFrames::DiscoveryResponseFrame;

        DiscoveryCommandFrame() {
            this->setProtocolHandlerId(ProtocolHandlerId::Discovery);
        }
    };
}

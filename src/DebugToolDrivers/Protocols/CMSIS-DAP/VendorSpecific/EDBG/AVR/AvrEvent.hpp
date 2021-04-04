#pragma once

#include <vector>

#include "src/DebugToolDrivers/Protocols/CMSIS-DAP/Response.hpp"
#include "src/DebugToolDrivers/Protocols/CMSIS-DAP/VendorSpecific/EDBG/Edbg.hpp"

namespace Bloom::DebugToolDrivers::Protocols::CmsisDap::Edbg::Avr
{
    using Edbg::ProtocolHandlerId;

    enum class AvrEventId : unsigned char
    {
        AVR8_BREAK_EVENT = 0x40,
    };

    inline bool operator==(unsigned char rawId, AvrEventId id) {
        return static_cast<unsigned char>(id) == rawId;
    }

    inline bool operator==(AvrEventId id, unsigned char rawId) {
        return rawId == id;
    }

    class AvrEvent: public Response
    {
    private:
        unsigned char eventId;

        std::vector<unsigned char> eventData;

    protected:
        void setEventData(const std::vector<unsigned char>& eventData) {
            this->eventData = eventData;
        }

    public:
        AvrEvent() = default;

        /**
         * Construct an AVRResponse object from a Response object.
         *
         * @param response
         */
        void init(const Response& response) {
            auto rawData = response.getData();
            rawData.insert(rawData.begin(), response.getResponseId());
            this->init(rawData.data(), rawData.size());
        }

        void init(unsigned char* response, size_t length) override;

        const std::vector<unsigned char>& getEventData() const {
            return this->eventData;
        }

        size_t getEventDataSize() const {
            return this->eventData.size();
        }

        AvrEventId getEventId() {
            return static_cast<AvrEventId>(this->eventId);
        }
    };
}

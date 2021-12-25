#pragma once

#include <string>

#include "Event.hpp"
#include "TargetPinStatesRetrieved.hpp"
#include "src/Targets/TargetPinDescriptor.hpp"

namespace Bloom::Events
{
    class SetTargetPinState: public Event
    {
    public:
        using TargetControllerResponseType = TargetPinStatesRetrieved;

        static constexpr EventType type = EventType::SET_TARGET_PIN_STATE;
        static inline const std::string name = "SetTargetPinState";
        Targets::TargetPinDescriptor pinDescriptor;
        Targets::TargetPinState pinState;

        [[nodiscard]] EventType getType() const override {
            return SetTargetPinState::type;
        }

        [[nodiscard]] std::string getName() const override {
            return SetTargetPinState::name;
        }
    };
}

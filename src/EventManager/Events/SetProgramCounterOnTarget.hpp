#pragma once

#include <cstdint>
#include <string>

#include "Event.hpp"
#include "ProgramCounterSetOnTarget.hpp"

namespace Bloom::Events
{
    class SetProgramCounterOnTarget: public Event
    {
    public:
        using TargetControllerResponseType = ProgramCounterSetOnTarget;

        static inline EventType type = EventType::SET_PROGRAM_COUNTER_ON_TARGET;
        static inline const std::string name = "SetProgramCounterOnTarget";
        std::uint32_t address = 0;

        [[nodiscard]] EventType getType() const override {
            return SetProgramCounterOnTarget::type;
        }

        [[nodiscard]] std::string getName() const override {
            return SetProgramCounterOnTarget::name;
        }
    };
}

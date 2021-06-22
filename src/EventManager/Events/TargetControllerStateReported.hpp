#pragma once

#include <string>

#include "Event.hpp"
#include "src/TargetController/TargetControllerState.hpp"

namespace Bloom::Events
{
    class TargetControllerStateReported: public Event
    {
    public:
        TargetControllerState state;

        explicit TargetControllerStateReported(TargetControllerState state): state(state) {};

        static inline const std::string name = "TargetControllerStateReported";

        [[nodiscard]] std::string getName() const override {
            return TargetControllerStateReported::name;
        }
    };
}

#pragma once

#include <string>

#include "Event.hpp"

namespace Bloom::Events
{
    class StopTargetExecution: public Event
    {
    public:
        static inline const std::string name = "StopTargetEvent";

        [[nodiscard]] std::string getName() const override {
            return StopTargetExecution::name;
        }
    };
}

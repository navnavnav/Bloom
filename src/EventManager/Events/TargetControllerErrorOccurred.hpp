#pragma once

#include <string>

#include "Event.hpp"

namespace Bloom::Events
{
    class TargetControllerErrorOccurred: public Event
    {
    public:
        static constexpr EventType type = EventType::TARGET_CONTROLLER_ERROR_OCCURRED;
        static inline const std::string name = "TargetControllerErrorOccurred";

        std::string errorMessage;

        TargetControllerErrorOccurred() = default;
        TargetControllerErrorOccurred(const std::string& errorMessage): errorMessage(errorMessage) {};

        [[nodiscard]] EventType getType() const override {
            return TargetControllerErrorOccurred::type;
        }

        [[nodiscard]] std::string getName() const override {
            return TargetControllerErrorOccurred::name;
        }
    };
}

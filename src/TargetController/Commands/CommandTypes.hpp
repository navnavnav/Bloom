#pragma once

#include <cstdint>

namespace Bloom::TargetController::Commands
{
    enum class CommandType: std::uint8_t
    {
        GENERIC,
        STOP_TARGET_EXECUTION,
        RESUME_TARGET_EXECUTION,
        RESET_TARGET,
        READ_TARGET_REGISTERS,
    };
}

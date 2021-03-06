#pragma once

#include "InsightWorkerTask.hpp"

namespace Bloom
{
    class ReadStackPointer: public InsightWorkerTask
    {
        Q_OBJECT

    public:
        ReadStackPointer() = default;

    signals:
        void stackPointerRead(std::uint32_t stackPointer);

    protected:
        void run(TargetController::TargetControllerConsole& targetControllerConsole) override;
    };
}

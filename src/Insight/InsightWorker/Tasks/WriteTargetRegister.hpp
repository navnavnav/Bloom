#pragma once

#include "InsightWorkerTask.hpp"
#include "src/Targets/TargetRegister.hpp"

namespace Bloom
{
    class WriteTargetRegister: public InsightWorkerTask
    {
        Q_OBJECT

    public:
        explicit WriteTargetRegister(const Targets::TargetRegister& targetRegister): targetRegister(targetRegister) {}

    protected:
        void run(TargetController::TargetControllerConsole& targetControllerConsole) override;

    private:
        Targets::TargetRegister targetRegister;
    };
}

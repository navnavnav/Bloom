#include "ContinueExecution.hpp"

#include "src/DebugServer/Gdb/ResponsePackets/ErrorResponsePacket.hpp"

#include "src/Logger/Logger.hpp"
#include "src/Exceptions/Exception.hpp"

namespace Bloom::DebugServer::Gdb::CommandPackets
{
    using TargetController::TargetControllerConsole;

    using ResponsePackets::ErrorResponsePacket;
    using Exceptions::Exception;

    ContinueExecution::ContinueExecution(const RawPacketType& rawPacket)
        : CommandPacket(rawPacket)
    {
        if (this->data.size() > 1) {
            this->fromProgramCounter = static_cast<std::uint32_t>(
                std::stoi(std::string(this->data.begin(), this->data.end()), nullptr, 16)
            );
        }
    }

    void ContinueExecution::handle(DebugSession& debugSession, TargetControllerConsole& targetControllerConsole) {
        Logger::debug("Handling ContinueExecution packet");

        try {
            targetControllerConsole.continueTargetExecution(this->fromProgramCounter);
            debugSession.waitingForBreak = true;

        } catch (const Exception& exception) {
            Logger::error("Failed to continue execution on target - " + exception.getMessage());
            debugSession.connection.writePacket(ErrorResponsePacket());
        }
    }
}

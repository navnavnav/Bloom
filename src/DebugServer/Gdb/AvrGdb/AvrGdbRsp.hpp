#pragma once

#include <cstdint>

#include "TargetDescriptor.hpp"

#include "src/DebugServer/Gdb/GdbRspDebugServer.hpp"

namespace Bloom::DebugServer::Gdb::AvrGdb
{
    /**
     * The AVR GDB client (avr-gdb) defines a set of parameters relating to AVR targets. These parameters are
     * hardcoded in the AVR GDB source code. The client expects all compatible GDB RSP servers to be aware of
     * these parameters.
     *
     * An example of these hardcoded parameters is target registers and the order in which they are supplied; AVR GDB
     * clients expect 35 registers to be accessible via the server. 32 of these registers are general purpose CPU
     * registers. The GP registers are expected to be followed by the status register (SREG), stack pointer
     * register (SPH & SPL) and the program counter. These must all be given in a specific order, which is
     * pre-determined by the AVR GDB client. See AvrGdbRsp::getRegisterNumberToDescriptorMapping() for more.
     *
     * For more on this, see the AVR GDB source code at https://github.com/bminor/binutils-gdb/blob/master/gdb/avr-tdep.c
     *
     * The AvrGdpRsp class extends the generic GDB RSP debug server and implements these AVR specific parameters.
     */
    class AvrGdbRsp: public GdbRspDebugServer
    {
    public:
        AvrGdbRsp(
            const DebugServerConfig& debugServerConfig,
            EventListener& eventListener,
            EventFdNotifier& eventNotifier
        );

        std::string getName() const override {
            return "AVR GDB Remote Serial Protocol Debug Server";
        }

    protected:
        void init() override;

        const Gdb::TargetDescriptor& getGdbTargetDescriptor() override {
            return this->gdbTargetDescriptor.value();
        }

        std::unique_ptr<Gdb::CommandPackets::CommandPacket> resolveCommandPacket(
            const RawPacketType& rawPacket
        ) override;

        std::set<std::pair<Feature, std::optional<std::string>>> getSupportedFeatures() override ;

    private:
        std::optional<TargetDescriptor> gdbTargetDescriptor;
    };
}

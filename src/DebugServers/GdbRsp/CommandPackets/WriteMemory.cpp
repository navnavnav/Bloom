#include "WriteMemory.hpp"

#include "src/DebugServers/GdbRsp/GdbRspDebugServer.hpp"

using namespace Bloom::DebugServers::Gdb::CommandPackets;
using namespace Bloom::Exceptions;

void WriteMemory::init() {
    if (this->data.size() < 4) {
        throw Exception("Invalid packet length");
    }

    auto packetString = QString::fromLocal8Bit(
        reinterpret_cast<const char*>(this->data.data() + 1),
        static_cast<int>(this->data.size() - 1)
    );

    /*
     * The write memory ('M') packet consists of three segments, an address, a length and a buffer.
     * The address and length are separated by a comma character, and the buffer proceeds a colon character.
     */
    auto packetSegments = packetString.split(",");
    if (packetSegments.size() != 2) {
        throw Exception("Unexpected number of segments in packet data: " + std::to_string(packetSegments.size()));
    }

    bool conversionStatus = false;
    this->startAddress = packetSegments.at(0).toUInt(&conversionStatus, 16);

    if (!conversionStatus) {
        throw Exception("Failed to parse start address from write memory packet data");
    }

    auto lengthAndBufferSegments = packetSegments.at(1).split(":");
    if (lengthAndBufferSegments.size() != 2) {
        throw Exception("Unexpected number of segments in packet data: " + std::to_string(lengthAndBufferSegments.size()));
    }

    auto bufferSize = lengthAndBufferSegments.at(0).toUInt(&conversionStatus, 16);
    if (!conversionStatus) {
        throw Exception("Failed to parse write length from write memory packet data");
    }

    this->buffer = Packet::hexToData(lengthAndBufferSegments.at(1).toStdString());

    if (this->buffer.size() != bufferSize) {
        throw Exception("Buffer size does not match length value given in write memory packet");
    }
}

void WriteMemory::dispatchToHandler(Gdb::GdbRspDebugServer& gdbRspDebugServer) {
    gdbRspDebugServer.handleGdbPacket(*this);
}

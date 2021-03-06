#pragma once

#include <cstdint>

#include "MemoryRegion.hpp"

namespace Bloom
{
    enum class MemoryRegionDataType: std::uint8_t
    {
        UNKNOWN,
        UNSIGNED_INTEGER,
        SIGNED_INTEGER,
        ASCII_STRING,
    };

    class FocusedMemoryRegion: public MemoryRegion
    {
    public:
        MemoryRegionDataType dataType = MemoryRegionDataType::UNKNOWN;
        Targets::TargetMemoryEndianness endianness = Targets::TargetMemoryEndianness::LITTLE;

        explicit FocusedMemoryRegion(
            const QString& name,
            const Targets::TargetMemoryAddressRange& addressRange
        ): MemoryRegion(name, MemoryRegionType::FOCUSED, addressRange) {};
    };
}

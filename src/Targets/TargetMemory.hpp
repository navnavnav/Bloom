#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace Bloom::Targets
{
    enum class TargetMemoryEndianness: std::uint8_t
    {
        BIG,
        LITTLE,
    };

    enum class TargetMemoryType: std::uint8_t
    {
        FLASH,
        RAM,
        EEPROM,
        OTHER,
    };

    struct TargetMemoryAddressRange
    {
        std::uint32_t startAddress = 0;
        std::uint32_t endAddress = 0;

        TargetMemoryAddressRange() = default;
        TargetMemoryAddressRange(std::uint32_t startAddress, std::uint32_t endAddress)
            : startAddress(startAddress)
            , endAddress(endAddress)
        {};

        bool operator == (const TargetMemoryAddressRange& rhs) const {
            return this->startAddress == rhs.startAddress && this->endAddress == rhs.endAddress;
        }

        bool operator < (const TargetMemoryAddressRange& rhs) const {
            return this->startAddress < rhs.startAddress;
        }

        [[nodiscard]] bool intersectsWith(const TargetMemoryAddressRange& other) const {
            return
                (other.startAddress <= this->startAddress && other.endAddress >= this->startAddress)
                || (other.endAddress >= this->endAddress && other.startAddress <= this->endAddress)
            ;
        }

        [[nodiscard]] bool contains(std::uint32_t address) const {
            return address >= this->startAddress && address <= this->endAddress;
        }

        [[nodiscard]] bool contains(const TargetMemoryAddressRange& addressRange) const {
            return this->startAddress <= addressRange.startAddress && this->endAddress >= addressRange.endAddress;
        }
    };

    struct TargetMemoryDescriptor
    {
        TargetMemoryType type;
        TargetMemoryAddressRange addressRange;
        std::optional<std::uint32_t> pageSize;

        TargetMemoryDescriptor(
            TargetMemoryType type,
            TargetMemoryAddressRange addressRange,
            std::optional<std::uint32_t> pageSize = std::nullopt
        )
            : type(type)
            , addressRange(addressRange)
            , pageSize(pageSize)
        {};

        bool operator == (const TargetMemoryDescriptor& rhs) const {
            return this->type == rhs.type && this->addressRange == rhs.addressRange;
        }

        [[nodiscard]] std::uint32_t size() const {
            return (this->addressRange.endAddress - this->addressRange.startAddress) + 1;
        }
    };

    using TargetMemoryBuffer = std::vector<unsigned char>;
}

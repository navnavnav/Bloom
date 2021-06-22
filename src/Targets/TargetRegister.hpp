#pragma once

#include <utility>
#include <vector>
#include <map>

namespace Bloom::Targets
{
    enum class TargetRegisterType: int
    {
        GENERAL_PURPOSE_REGISTER,
        PROGRAM_COUNTER,
        STACK_POINTER,
        STATUS_REGISTER,
    };

    struct TargetRegisterDescriptor
    {
        using IdType = size_t;
        std::optional<IdType> id;
        TargetRegisterType type = TargetRegisterType::GENERAL_PURPOSE_REGISTER;

        TargetRegisterDescriptor() = default;
        explicit TargetRegisterDescriptor(TargetRegisterType type): type(type) {};
        TargetRegisterDescriptor(IdType id, TargetRegisterType type): id(id), type(type) {};

        bool operator==(const TargetRegisterDescriptor& other) const {
            return this->id == other.id && this->type == other.type;
        }
    };

    struct TargetRegister
    {
        std::optional<size_t> id;
        std::vector<unsigned char> value;
        TargetRegisterDescriptor descriptor;

        explicit TargetRegister(std::vector<unsigned char> value): value(std::move(value)) {};

        TargetRegister(TargetRegisterDescriptor descriptor, std::vector<unsigned char> value): value(std::move(value)),
        descriptor(descriptor) {};

        TargetRegister(size_t id, std::vector<unsigned char> value): id(id), value(std::move(value)) {
            this->descriptor.id = id;
        };

        [[nodiscard]] size_t size() const {
            return this->value.size();
        }
    };

    using TargetRegisterMap = std::map<size_t, TargetRegister>;
    using TargetRegisters = std::vector<TargetRegister>;
    using TargetRegisterDescriptors = std::vector<TargetRegisterDescriptor>;
}

namespace std {
    /**
     * Hashing function for TargetRegisterDescriptor type.
     *
     * This is required in order to use TargetRegisterDescriptor as a key in an std::unordered_map (see the BiMap
     * class)
     */
    template<>
    class hash<Bloom::Targets::TargetRegisterDescriptor> {
    public:
        size_t operator()(const Bloom::Targets::TargetRegisterDescriptor& descriptor) const {
            return descriptor.id.value_or(0) + static_cast<size_t>(descriptor.type);
        }
    };
}

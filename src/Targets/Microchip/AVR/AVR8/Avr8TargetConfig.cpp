#include "Avr8TargetConfig.hpp"

#include "src/Helpers/String.hpp"
#include "src/Exceptions/InvalidConfig.hpp"

namespace Bloom::Targets::Microchip::Avr::Avr8Bit
{
    Avr8TargetConfig::Avr8TargetConfig(const TargetConfig& targetConfig): TargetConfig(targetConfig) {
        using Bloom::Exceptions::InvalidConfig;

        const auto& targetNode = targetConfig.targetNode;

        if (!targetNode["physicalInterface"]) {
            throw InvalidConfig("Missing physical interface config parameter for AVR8 target.");
        }

        const auto physicalInterfaceName = String::asciiToLower(targetNode["physicalInterface"].as<std::string>());

        static const auto physicalInterfacesByName = Avr8TargetConfig::getPhysicalInterfacesByName();

        if (!physicalInterfacesByName.contains(physicalInterfaceName)) {
            throw InvalidConfig("Invalid physical interface config parameter for AVR8 target.");
        }

        this->physicalInterface = physicalInterfacesByName.at(physicalInterfaceName);

        if (targetNode["updateDwenFuseBit"]) {
            this->updateDwenFuseBit = targetNode["updateDwenFuseBit"].as<bool>();
        }

        if (targetNode["cycleTargetPowerPostDwenUpdate"]) {
            this->cycleTargetPowerPostDwenUpdate = targetNode["cycleTargetPowerPostDwenUpdate"].as<bool>();
        }

        if (targetNode["disableDebugWirePreDisconnect"]) {
            this->disableDebugWireOnDeactivate = targetNode["disableDebugWirePreDisconnect"].as<bool>();
        }

        if (targetNode["targetPowerCycleDelay"]) {
            this->targetPowerCycleDelay = std::chrono::milliseconds(targetNode["targetPowerCycleDelay"].as<int>());
        }
    }
}

#include "TargetControllerComponent.hpp"

#include <thread>
#include <filesystem>
#include <typeindex>
#include <algorithm>

#include "src/Application.hpp"
#include "src/Helpers/Paths.hpp"
#include "src/Logger/Logger.hpp"

#include "src/TargetController/Exceptions/DeviceFailure.hpp"
#include "src/TargetController/Exceptions/TargetOperationFailure.hpp"
#include "src/Exceptions/TargetControllerStartupFailure.hpp"
#include "src/Exceptions/InvalidConfig.hpp"

namespace Bloom::TargetController
{
    using namespace Bloom::Targets;
    using namespace Bloom::Events;
    using namespace Bloom::Exceptions;

    using Commands::CommandIdType;

    using Commands::Command;
    using Commands::GetTargetDescriptor;
    using Commands::GetTargetState;
    using Commands::StopTargetExecution;
    using Commands::ResumeTargetExecution;
    using Commands::ResetTarget;
    using Commands::ReadTargetRegisters;
    using Commands::WriteTargetRegisters;
    using Commands::ReadTargetMemory;
    using Commands::WriteTargetMemory;
    using Commands::StepTargetExecution;
    using Commands::SetBreakpoint;
    using Commands::RemoveBreakpoint;
    using Commands::SetTargetProgramCounter;
    using Commands::GetTargetPinStates;
    using Commands::SetTargetPinState;
    using Commands::GetTargetStackPointer;
    using Commands::GetTargetProgramCounter;
    using Commands::EnableProgrammingMode;
    using Commands::DisableProgrammingMode;

    using Responses::Response;
    using Responses::TargetRegistersRead;
    using Responses::TargetMemoryRead;
    using Responses::TargetPinStates;
    using Responses::TargetStackPointer;
    using Responses::TargetProgramCounter;

    TargetControllerComponent::TargetControllerComponent(
        const ProjectConfig& projectConfig,
        const EnvironmentConfig& environmentConfig
    )
        : projectConfig(projectConfig)
        , environmentConfig(environmentConfig)
    {}

    void TargetControllerComponent::run() {
        try {
            this->startup();

            this->setThreadStateAndEmitEvent(ThreadState::READY);
            Logger::debug("TargetController ready and waiting for events.");

            while (this->getThreadState() == ThreadState::READY) {
                try {
                    if (this->state == TargetControllerState::ACTIVE) {
                        this->fireTargetEvents();
                    }

                    TargetControllerComponent::notifier.waitForNotification(std::chrono::milliseconds(60));

                    this->processQueuedCommands();
                    this->eventListener->dispatchCurrentEvents();

                } catch (const DeviceFailure& exception) {
                    /*
                     * Upon a device failure, we assume Bloom has lost control of the debug tool. This could be the
                     * result of the user disconnecting the debug tool, or issuing a soft reset. The soft reset could
                     * have been issued via another application, without the user's knowledge.
                     * See https://github.com/navnavnav/Bloom/issues/3 for more on that.
                     *
                     * The TC will go into a suspended state and the DebugServer should terminate any active debug
                     * session. When the user attempts to start another debug session, we will try to re-connect to the
                     * debug tool.
                     */
                    Logger::error("Device failure detected - " + exception.getMessage());
                    Logger::error("Suspending TargetController");
                    this->suspend();
                }
            }

        } catch (const TargetControllerStartupFailure& exception) {
            Logger::error("TargetController failed to startup. See below for errors:");
            Logger::error(exception.getMessage());

        } catch (const Exception& exception) {
            Logger::error("The TargetController encountered a fatal error. See below for errors:");
            Logger::error(exception.getMessage());

        } catch (const std::exception& exception) {
            Logger::error("The TargetController encountered a fatal error. See below for errors:");
            Logger::error(std::string(exception.what()));
        }

        this->shutdown();
    }

    TargetControllerState TargetControllerComponent::getState() {
        return TargetControllerComponent::state;
    }

    void TargetControllerComponent::registerCommand(std::unique_ptr<Command> command) {
        auto commandQueueLock = TargetControllerComponent::commandQueue.acquireLock();
        TargetControllerComponent::commandQueue.getValue().push(std::move(command));
        TargetControllerComponent::notifier.notify();
    }

    std::optional<std::unique_ptr<Responses::Response>> TargetControllerComponent::waitForResponse(
        CommandIdType commandId,
        std::optional<std::chrono::milliseconds> timeout
    ) {
        auto response = std::unique_ptr<Response>(nullptr);

        const auto predicate = [commandId, &response] {
            auto& responsesByCommandId = TargetControllerComponent::responsesByCommandId.getValue();
            auto responseIt = responsesByCommandId.find(commandId);

            if (responseIt != responsesByCommandId.end()) {
                response.swap(responseIt->second);
                responsesByCommandId.erase(responseIt);

                return true;
            }

            return false;
        };

        auto responsesByCommandIdLock = TargetControllerComponent::responsesByCommandId.acquireLock();

        if (timeout.has_value()) {
            TargetControllerComponent::responsesByCommandIdCv.wait_for(
                responsesByCommandIdLock,
                timeout.value(),
                predicate
            );

        } else {
            TargetControllerComponent::responsesByCommandIdCv.wait(responsesByCommandIdLock, predicate);
        }

        return (response != nullptr) ? std::optional(std::move(response)) : std::nullopt;
    }

    void TargetControllerComponent::deregisterCommandHandler(Commands::CommandType commandType) {
        this->commandHandlersByCommandType.erase(commandType);
    }

    void TargetControllerComponent::startup() {
        this->setName("TC");
        Logger::info("Starting TargetController");
        this->setThreadState(ThreadState::STARTING);
        this->blockAllSignals();
        this->eventListener->setInterruptEventNotifier(&TargetControllerComponent::notifier);
        EventManager::registerListener(this->eventListener);

        // Install Bloom's udev rules if not already installed
        TargetControllerComponent::checkUdevRules();

        // Register event handlers
        this->eventListener->registerCallbackForEventType<Events::ShutdownTargetController>(
            std::bind(&TargetControllerComponent::onShutdownTargetControllerEvent, this, std::placeholders::_1)
        );

        this->eventListener->registerCallbackForEventType<Events::DebugSessionStarted>(
            std::bind(&TargetControllerComponent::onDebugSessionStartedEvent, this, std::placeholders::_1)
        );

        this->resume();
    }

    std::map<
        std::string,
        std::function<std::unique_ptr<DebugTool>()>
    > TargetControllerComponent::getSupportedDebugTools() {
        // The debug tool names in this mapping should always be lower-case.
        return std::map<std::string, std::function<std::unique_ptr<DebugTool>()>> {
            {
                "atmel-ice",
                [] {
                    return std::make_unique<DebugToolDrivers::AtmelIce>();
                }
            },
            {
                "power-debugger",
                [] {
                    return std::make_unique<DebugToolDrivers::PowerDebugger>();
                }
            },
            {
                "snap",
                [] {
                    return std::make_unique<DebugToolDrivers::MplabSnap>();
                }
            },
            {
                "pickit-4",
                [] {
                    return std::make_unique<DebugToolDrivers::MplabPickit4>();
                }
            },
            {
                "xplained-pro",
                [] {
                    return std::make_unique<DebugToolDrivers::XplainedPro>();
                }
            },
            {
                "xplained-mini",
                [] {
                    return std::make_unique<DebugToolDrivers::XplainedMini>();
                }
            },
            {
                "xplained-nano",
                [] {
                    return std::make_unique<DebugToolDrivers::XplainedNano>();
                }
            },
            {
                "curiosity-nano",
                [] {
                    return std::make_unique<DebugToolDrivers::CuriosityNano>();
                }
            },
            {
                "jtagice3",
                [] {
                    return std::make_unique<DebugToolDrivers::JtagIce3>();
                }
            },
        };
    }

    std::map<
        std::string,
        std::function<std::unique_ptr<Targets::Target>()>
    > TargetControllerComponent::getSupportedTargets() {
        using Avr8TargetDescriptionFile = Targets::Microchip::Avr::Avr8Bit::TargetDescription::TargetDescriptionFile;

        auto mapping = std::map<std::string, std::function<std::unique_ptr<Targets::Target>()>>({
            {
                "avr8",
                [] {
                    return std::make_unique<Targets::Microchip::Avr::Avr8Bit::Avr8>();
                }
            },
        });

        // Include all targets from AVR8 target description files
        const auto avr8PdMapping = Avr8TargetDescriptionFile::getTargetDescriptionMapping();

        for (auto mapIt = avr8PdMapping.begin(); mapIt != avr8PdMapping.end(); mapIt++) {
            // Each target signature maps to an array of targets, as numerous targets can possess the same signature.
            auto targets = mapIt.value().toArray();

            for (auto targetIt = targets.begin(); targetIt != targets.end(); targetIt++) {
                auto targetName = targetIt->toObject().find("targetName").value().toString()
                    .toLower().toStdString();
                auto targetSignatureHex = mapIt.key().toLower().toStdString();

                if (!mapping.contains(targetName)) {
                    mapping.insert({
                        targetName,
                        [targetName, targetSignatureHex] {
                            return std::make_unique<Targets::Microchip::Avr::Avr8Bit::Avr8>(
                                targetName,
                                Targets::Microchip::Avr::TargetSignature(targetSignatureHex)
                            );
                        }
                    });
                }
            }
        }

        return mapping;
    }

    void TargetControllerComponent::processQueuedCommands() {
        auto commands = std::queue<std::unique_ptr<Command>>();

        {
            const auto queueLock = TargetControllerComponent::commandQueue.acquireLock();
            commands.swap(TargetControllerComponent::commandQueue.getValue());
        }

        while (!commands.empty()) {
            const auto command = std::move(commands.front());
            commands.pop();

            const auto commandId = command->id;
            const auto commandType = command->getType();

            try {
                if (!this->commandHandlersByCommandType.contains(commandType)) {
                    throw Exception("No handler registered for this command.");
                }

                if (command->requiresStoppedTargetState() && this->lastTargetState != TargetState::STOPPED) {
                    throw Exception("Illegal target state - command requires target to be stopped");
                }

                if (this->target->programmingModeEnabled() && command->requiresDebugMode()) {
                    throw Exception(
                        "Illegal target state - command cannot be serviced whilst the target is in programming mode."
                    );
                }

                this->registerCommandResponse(
                    commandId,
                    this->commandHandlersByCommandType.at(commandType)(*(command.get()))
                );

            } catch (const Exception& exception) {
                this->registerCommandResponse(
                    commandId,
                    std::make_unique<Responses::Error>(exception.getMessage())
                );
            }
        }
    }

    void TargetControllerComponent::registerCommandResponse(
        CommandIdType commandId,
        std::unique_ptr<Response> response
    ) {
        const auto responseMappingLock = TargetControllerComponent::responsesByCommandId.acquireLock();
        TargetControllerComponent::responsesByCommandId.getValue().insert(
            std::pair(commandId, std::move(response))
        );
        TargetControllerComponent::responsesByCommandIdCv.notify_all();
    }

    void TargetControllerComponent::checkUdevRules() {
        auto bloomRulesPath = std::string("/etc/udev/rules.d/99-bloom.rules");
        auto latestBloomRulesPath = Paths::resourcesDirPath() + "/UDevRules/99-bloom.rules";

        if (!std::filesystem::exists(bloomRulesPath)) {
            Logger::warning("Bloom udev rules missing - attempting installation");

            // We can only install them if we're running as root
            if (!Application::isRunningAsRoot()) {
                Logger::error("Bloom udev rules missing - cannot install udev rules without root privileges.\n"
                    "Running Bloom once with root privileges will allow it to automatically install the udev rules. "
                    "Alternatively, instructions on manually installing the udev rules can be found "
                    "here: " + Paths::homeDomainName() + "/docs/getting-started\nBloom may fail to connect to some "
                    "debug tools until this is resolved.");
                return;
            }

            if (!std::filesystem::exists(latestBloomRulesPath)) {
                // This shouldn't happen, but it can if someone has been messing with the installation files
                Logger::error(
                    "Unable to install Bloom udev rules - \"" + latestBloomRulesPath + "\" does not exist."
                );
                return;
            }

            std::filesystem::copy(latestBloomRulesPath, bloomRulesPath);
            Logger::warning("Bloom udev rules installed - a reconnect of the debug tool may be required "
                "before the new udev rules come into effect.");
        }
    }

    void TargetControllerComponent::shutdown() {
        if (this->getThreadState() == ThreadState::STOPPED) {
            return;
        }

        try {
            Logger::info("Shutting down TargetController");
            EventManager::deregisterListener(this->eventListener->getId());
            this->releaseHardware();

        } catch (const std::exception& exception) {
            this->target.reset();
            this->debugTool.reset();
            Logger::error(
                "Failed to properly shutdown TargetController. Error: " + std::string(exception.what())
            );
        }

        this->setThreadStateAndEmitEvent(ThreadState::STOPPED);
    }

    void TargetControllerComponent::suspend() {
        if (this->getThreadState() != ThreadState::READY) {
            return;
        }

        Logger::debug("Suspending TargetController");

        try {
            this->releaseHardware();

        } catch (const std::exception& exception) {
            Logger::error("Failed to release connected debug tool and target resources. Error: "
                + std::string(exception.what()));
        }

        this->deregisterCommandHandler(GetTargetDescriptor::type);
        this->deregisterCommandHandler(GetTargetState::type);
        this->deregisterCommandHandler(StopTargetExecution::type);
        this->deregisterCommandHandler(ResumeTargetExecution::type);
        this->deregisterCommandHandler(ResetTarget::type);
        this->deregisterCommandHandler(ReadTargetRegisters::type);
        this->deregisterCommandHandler(WriteTargetRegisters::type);
        this->deregisterCommandHandler(ReadTargetMemory::type);
        this->deregisterCommandHandler(WriteTargetMemory::type);
        this->deregisterCommandHandler(StepTargetExecution::type);
        this->deregisterCommandHandler(SetBreakpoint::type);
        this->deregisterCommandHandler(RemoveBreakpoint::type);
        this->deregisterCommandHandler(SetTargetProgramCounter::type);
        this->deregisterCommandHandler(GetTargetPinStates::type);
        this->deregisterCommandHandler(SetTargetPinState::type);
        this->deregisterCommandHandler(GetTargetStackPointer::type);
        this->deregisterCommandHandler(GetTargetProgramCounter::type);
        this->deregisterCommandHandler(EnableProgrammingMode::type);
        this->deregisterCommandHandler(DisableProgrammingMode::type);

        this->eventListener->deregisterCallbacksForEventType<Events::DebugSessionFinished>();

        this->lastTargetState = TargetState::UNKNOWN;
        this->cachedTargetDescriptor = std::nullopt;
        this->registerDescriptorsByMemoryType.clear();
        this->registerAddressRangeByMemoryType.clear();

        TargetControllerComponent::state = TargetControllerState::SUSPENDED;
        EventManager::triggerEvent(std::make_shared<TargetControllerStateChanged>(TargetControllerComponent::state));

        Logger::debug("TargetController suspended");
    }

    void TargetControllerComponent::resume() {
        this->acquireHardware();
        this->loadRegisterDescriptors();

        this->registerCommandHandler<GetTargetDescriptor>(
            std::bind(&TargetControllerComponent::handleGetTargetDescriptor, this, std::placeholders::_1)
        );

        this->registerCommandHandler<GetTargetState>(
            std::bind(&TargetControllerComponent::handleGetTargetState, this, std::placeholders::_1)
        );

        this->registerCommandHandler<StopTargetExecution>(
            std::bind(&TargetControllerComponent::handleStopTargetExecution, this, std::placeholders::_1)
        );

        this->registerCommandHandler<ResumeTargetExecution>(
            std::bind(&TargetControllerComponent::handleResumeTargetExecution, this, std::placeholders::_1)
        );

        this->registerCommandHandler<ResetTarget>(
            std::bind(&TargetControllerComponent::handleResetTarget, this, std::placeholders::_1)
        );

        this->registerCommandHandler<ReadTargetRegisters>(
            std::bind(&TargetControllerComponent::handleReadTargetRegisters, this, std::placeholders::_1)
        );

        this->registerCommandHandler<WriteTargetRegisters>(
            std::bind(&TargetControllerComponent::handleWriteTargetRegisters, this, std::placeholders::_1)
        );

        this->registerCommandHandler<ReadTargetMemory>(
            std::bind(&TargetControllerComponent::handleReadTargetMemory, this, std::placeholders::_1)
        );

        this->registerCommandHandler<WriteTargetMemory>(
            std::bind(&TargetControllerComponent::handleWriteTargetMemory, this, std::placeholders::_1)
        );

        this->registerCommandHandler<StepTargetExecution>(
            std::bind(&TargetControllerComponent::handleStepTargetExecution, this, std::placeholders::_1)
        );

        this->registerCommandHandler<SetBreakpoint>(
            std::bind(&TargetControllerComponent::handleSetBreakpoint, this, std::placeholders::_1)
        );

        this->registerCommandHandler<RemoveBreakpoint>(
            std::bind(&TargetControllerComponent::handleRemoveBreakpoint, this, std::placeholders::_1)
        );

        this->registerCommandHandler<SetTargetProgramCounter>(
            std::bind(&TargetControllerComponent::handleSetProgramCounter, this, std::placeholders::_1)
        );

        this->registerCommandHandler<GetTargetPinStates>(
            std::bind(&TargetControllerComponent::handleGetTargetPinStates, this, std::placeholders::_1)
        );

        this->registerCommandHandler<SetTargetPinState>(
            std::bind(&TargetControllerComponent::handleSetTargetPinState, this, std::placeholders::_1)
        );

        this->registerCommandHandler<GetTargetStackPointer>(
            std::bind(&TargetControllerComponent::handleGetTargetStackPointer, this, std::placeholders::_1)
        );

        this->registerCommandHandler<GetTargetProgramCounter>(
            std::bind(&TargetControllerComponent::handleGetTargetProgramCounter, this, std::placeholders::_1)
        );

        this->registerCommandHandler<EnableProgrammingMode>(
            std::bind(&TargetControllerComponent::handleEnableProgrammingMode, this, std::placeholders::_1)
        );

        this->registerCommandHandler<DisableProgrammingMode>(
            std::bind(&TargetControllerComponent::handleDisableProgrammingMode, this, std::placeholders::_1)
        );

        this->eventListener->registerCallbackForEventType<Events::DebugSessionFinished>(
            std::bind(&TargetControllerComponent::onDebugSessionFinishedEvent, this, std::placeholders::_1)
        );

        TargetControllerComponent::state = TargetControllerState::ACTIVE;
        EventManager::triggerEvent(
            std::make_shared<TargetControllerStateChanged>(TargetControllerComponent::state)
        );

        if (this->target->getState() != TargetState::RUNNING) {
            this->target->run();
            this->lastTargetState = TargetState::RUNNING;
        }
    }

    void TargetControllerComponent::acquireHardware() {
        auto debugToolName = this->environmentConfig.debugToolConfig.name;
        auto targetName = this->environmentConfig.targetConfig.name;

        static auto supportedDebugTools = this->getSupportedDebugTools();
        static auto supportedTargets = this->getSupportedTargets();

        if (!supportedDebugTools.contains(debugToolName)) {
            throw Exceptions::InvalidConfig(
                "Debug tool name (\"" + debugToolName + "\") not recognised. Please check your configuration!"
            );
        }

        if (!supportedTargets.contains(targetName)) {
            throw Exceptions::InvalidConfig(
                "Target name (\"" + targetName + "\") not recognised. Please check your configuration!"
            );
        }

        // Initiate debug tool and target
        this->debugTool = supportedDebugTools.at(debugToolName)();

        Logger::info("Connecting to debug tool");
        this->debugTool->init();

        Logger::info("Debug tool connected");
        Logger::info("Debug tool name: " + this->debugTool->getName());
        Logger::info("Debug tool serial: " + this->debugTool->getSerialNumber());

        this->target = supportedTargets.at(targetName)();

        if (!this->target->isDebugToolSupported(this->debugTool.get())) {
            throw Exceptions::InvalidConfig(
                "Debug tool (\"" + this->debugTool->getName() + "\") not supported " +
                    "by target (\"" + this->target->getName() + "\")."
            );
        }

        this->target->setDebugTool(this->debugTool.get());
        this->target->preActivationConfigure(this->environmentConfig.targetConfig);

        Logger::info("Activating target");
        this->target->activate();
        Logger::info("Target activated");
        this->target->postActivationConfigure();

        while (this->target->supportsPromotion()) {
            auto promotedTarget = this->target->promote();

            if (
                promotedTarget == nullptr
                || std::type_index(typeid(*promotedTarget)) == std::type_index(typeid(*this->target))
            ) {
                break;
            }

            this->target = std::move(promotedTarget);
            this->target->postPromotionConfigure();
        }

        Logger::info("Target ID: " + this->target->getHumanReadableId());
        Logger::info("Target name: " + this->target->getName());
    }

    void TargetControllerComponent::releaseHardware() {
        /*
         * Transferring ownership of this->debugTool and this->target to this function block means if an exception is
         * thrown, the objects will still be destroyed.
         */
        auto debugTool = std::move(this->debugTool);
        auto target = std::move(this->target);

        if (debugTool != nullptr && debugTool->isInitialised()) {
            if (target != nullptr) {
                /*
                 * We call deactivate() without checking if the target is activated. This will address any cases
                 * where a target is only partially activated (where the call to activate() failed).
                 */
                Logger::info("Deactivating target");
                target->deactivate();
            }

            Logger::info("Closing debug tool");
            debugTool->close();
        }
    }

    void TargetControllerComponent::loadRegisterDescriptors() {
        auto& targetDescriptor = this->getTargetDescriptor();

        for (const auto& [registerType, registerDescriptors] : targetDescriptor.registerDescriptorsByType) {
            for (const auto& registerDescriptor : registerDescriptors) {
                auto startAddress = registerDescriptor.startAddress.value_or(0);
                auto endAddress = startAddress + (registerDescriptor.size - 1);

                if (!this->registerAddressRangeByMemoryType.contains(registerDescriptor.memoryType)) {
                    auto addressRange = TargetMemoryAddressRange();
                    addressRange.startAddress = startAddress;
                    addressRange.endAddress = endAddress;
                    this->registerAddressRangeByMemoryType.insert(
                        std::pair(registerDescriptor.memoryType, addressRange)
                    );

                } else {
                    auto& addressRange = this->registerAddressRangeByMemoryType.at(registerDescriptor.memoryType);

                    if (startAddress < addressRange.startAddress) {
                        addressRange.startAddress = startAddress;
                    }

                    if (endAddress > addressRange.endAddress) {
                        addressRange.endAddress = endAddress;
                    }
                }

                this->registerDescriptorsByMemoryType[registerDescriptor.memoryType].insert(registerDescriptor);
            }
        }
    }

    TargetRegisterDescriptors TargetControllerComponent::getRegisterDescriptorsWithinAddressRange(
        std::uint32_t startAddress,
        std::uint32_t endAddress,
        Targets::TargetMemoryType memoryType
    ) {
        auto output = TargetRegisterDescriptors();

        if (this->registerAddressRangeByMemoryType.contains(memoryType)
            && this->registerDescriptorsByMemoryType.contains(memoryType)
        ) {
            auto& registersAddressRange = this->registerAddressRangeByMemoryType.at(memoryType);

            if (
                (startAddress <= registersAddressRange.startAddress && endAddress >= registersAddressRange.startAddress)
                || (startAddress <= registersAddressRange.endAddress && endAddress >= registersAddressRange.startAddress)
            ) {
                auto& registerDescriptors = this->registerDescriptorsByMemoryType.at(memoryType);

                for (const auto& registerDescriptor : registerDescriptors) {
                    if (!registerDescriptor.startAddress.has_value() || registerDescriptor.size < 1) {
                        continue;
                    }

                    auto registerStartAddress = registerDescriptor.startAddress.value();
                    auto registerEndAddress = registerStartAddress + registerDescriptor.size;

                    if (
                        (startAddress <= registerStartAddress && endAddress >= registerStartAddress)
                        || (startAddress <= registerEndAddress && endAddress >= registerStartAddress)
                    ) {
                        output.insert(registerDescriptor);
                    }
                }
            }
        }

        return output;
    }

    void TargetControllerComponent::fireTargetEvents() {
        auto newTargetState = this->target->getState();

        if (newTargetState != this->lastTargetState) {
            this->lastTargetState = newTargetState;

            if (newTargetState == TargetState::STOPPED) {
                Logger::debug("Target state changed - STOPPED");
                EventManager::triggerEvent(std::make_shared<TargetExecutionStopped>(
                    this->target->getProgramCounter(),
                    TargetBreakCause::UNKNOWN
                ));
            }

            if (newTargetState == TargetState::RUNNING) {
                Logger::debug("Target state changed - RUNNING");
                EventManager::triggerEvent(std::make_shared<TargetExecutionResumed>());
            }
        }
    }

    void TargetControllerComponent::resetTarget() {
        this->target->reset();

        EventManager::triggerEvent(std::make_shared<Events::TargetReset>());
    }

    void TargetControllerComponent::enableProgrammingMode() {
        Logger::debug("Enabling programming mode");
        this->target->enableProgrammingMode();
        Logger::warning("Programming mode enabled");

        EventManager::triggerEvent(std::make_shared<Events::ProgrammingModeEnabled>());
    }

    void TargetControllerComponent::disableProgrammingMode() {
        Logger::debug("Disabling programming mode");
        this->target->disableProgrammingMode();
        Logger::info("Programming mode disabled");

        EventManager::triggerEvent(std::make_shared<Events::ProgrammingModeDisabled>());
    }

    Targets::TargetDescriptor& TargetControllerComponent::getTargetDescriptor() {
        if (!this->cachedTargetDescriptor.has_value()) {
            this->cachedTargetDescriptor = this->target->getDescriptor();
        }

        return this->cachedTargetDescriptor.value();
    }

    void TargetControllerComponent::onShutdownTargetControllerEvent(const Events::ShutdownTargetController&) {
        this->shutdown();
    }

    void TargetControllerComponent::onDebugSessionStartedEvent(const Events::DebugSessionStarted&) {
        if (TargetControllerComponent::state == TargetControllerState::SUSPENDED) {
            Logger::debug("Waking TargetController");

            this->resume();
            this->fireTargetEvents();
        }
    }

    void TargetControllerComponent::onDebugSessionFinishedEvent(const DebugSessionFinished&) {
        if (this->target->getState() != TargetState::RUNNING) {
            this->target->run();
            this->fireTargetEvents();
        }

        if (this->environmentConfig.debugToolConfig.releasePostDebugSession) {
            this->suspend();
        }
    }

    std::unique_ptr<Responses::TargetDescriptor> TargetControllerComponent::handleGetTargetDescriptor(
        GetTargetDescriptor& command
    ) {
        return std::make_unique<Responses::TargetDescriptor>(this->getTargetDescriptor());
    }

    std::unique_ptr<Responses::TargetState> TargetControllerComponent::handleGetTargetState(GetTargetState& command) {
        return std::make_unique<Responses::TargetState>(this->target->getState());
    }

    std::unique_ptr<Response> TargetControllerComponent::handleStopTargetExecution(StopTargetExecution& command) {
        if (this->target->getState() != TargetState::STOPPED) {
            this->target->stop();
            this->lastTargetState = TargetState::STOPPED;
        }

        EventManager::triggerEvent(std::make_shared<Events::TargetExecutionStopped>(
            this->target->getProgramCounter(),
            TargetBreakCause::UNKNOWN
        ));

        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleResumeTargetExecution(
        ResumeTargetExecution& command
    ) {
        if (this->target->getState() != TargetState::RUNNING) {
            if (command.fromProgramCounter.has_value()) {
                this->target->setProgramCounter(command.fromProgramCounter.value());
            }

            this->target->run();
            this->lastTargetState = TargetState::RUNNING;
        }

        EventManager::triggerEvent(std::make_shared<Events::TargetExecutionResumed>());

        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleResetTarget(ResetTarget& command) {
        this->resetTarget();
        return std::make_unique<Response>();
    }

    std::unique_ptr<TargetRegistersRead> TargetControllerComponent::handleReadTargetRegisters(
        ReadTargetRegisters& command
    ) {
        return std::make_unique<TargetRegistersRead>(this->target->readRegisters(command.descriptors));
    }

    std::unique_ptr<Response> TargetControllerComponent::handleWriteTargetRegisters(WriteTargetRegisters& command) {
        this->target->writeRegisters(command.registers);

        auto registersWrittenEvent = std::make_shared<Events::RegistersWrittenToTarget>();
        registersWrittenEvent->registers = command.registers;

        EventManager::triggerEvent(registersWrittenEvent);

        return std::make_unique<Response>();
    }

    std::unique_ptr<TargetMemoryRead> TargetControllerComponent::handleReadTargetMemory(ReadTargetMemory& command) {
        return std::make_unique<TargetMemoryRead>(this->target->readMemory(
            command.memoryType,
            command.startAddress,
            command.bytes,
            command.excludedAddressRanges
        ));
    }

    std::unique_ptr<Response> TargetControllerComponent::handleWriteTargetMemory(WriteTargetMemory& command) {
        const auto& buffer = command.buffer;
        const auto bufferSize = command.buffer.size();
        const auto bufferStartAddress = command.startAddress;

        const auto& targetDescriptor = this->getTargetDescriptor();

        if (command.memoryType == targetDescriptor.programMemoryType && !this->target->programmingModeEnabled()) {
            throw Exception("Cannot write to program memory - programming mode not enabled.");
        }

        this->target->writeMemory(command.memoryType, bufferStartAddress, buffer);
        EventManager::triggerEvent(
            std::make_shared<Events::MemoryWrittenToTarget>(command.memoryType, bufferStartAddress, bufferSize)
        );

        if (
            EventManager::isEventTypeListenedFor(Events::RegistersWrittenToTarget::type)
            && command.memoryType == targetDescriptor.programMemoryType
            && this->registerDescriptorsByMemoryType.contains(command.memoryType)
        ) {
            /*
             * The memory type we just wrote to contains some number of registers - if we've written to any address
             * that is known to store the value of a register, trigger a RegistersWrittenToTarget event
             */
            const auto bufferEndAddress = static_cast<std::uint32_t>(bufferStartAddress + (bufferSize - 1));
            auto registerDescriptors = this->getRegisterDescriptorsWithinAddressRange(
                bufferStartAddress,
                bufferEndAddress,
                command.memoryType
            );

            if (!registerDescriptors.empty()) {
                auto registersWrittenEvent = std::make_shared<Events::RegistersWrittenToTarget>();

                for (const auto& registerDescriptor : registerDescriptors) {
                    const auto registerSize = registerDescriptor.size;
                    const auto registerStartAddress = registerDescriptor.startAddress.value();
                    const auto registerEndAddress = registerStartAddress + (registerSize - 1);

                    if (registerStartAddress < bufferStartAddress || registerEndAddress > bufferEndAddress) {
                        continue;
                    }

                    const auto bufferBeginIt = buffer.begin() + (registerStartAddress - bufferStartAddress);
                    registersWrittenEvent->registers.emplace_back(TargetRegister(
                        registerDescriptor,
                        TargetMemoryBuffer(bufferBeginIt, bufferBeginIt + registerSize)
                    ));
                }

                EventManager::triggerEvent(registersWrittenEvent);
            }
        }

        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleStepTargetExecution(StepTargetExecution& command) {
        if (command.fromProgramCounter.has_value()) {
            this->target->setProgramCounter(command.fromProgramCounter.value());
        }

        this->target->step();
        this->lastTargetState = TargetState::RUNNING;
        EventManager::triggerEvent(std::make_shared<Events::TargetExecutionResumed>());

        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleSetBreakpoint(SetBreakpoint& command) {
        this->target->setBreakpoint(command.breakpoint.address);
        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleRemoveBreakpoint(RemoveBreakpoint& command) {
        this->target->removeBreakpoint(command.breakpoint.address);
        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleSetProgramCounter(SetTargetProgramCounter& command) {
        this->target->setProgramCounter(command.address);
        return std::make_unique<Response>();
    }

    std::unique_ptr<TargetPinStates> TargetControllerComponent::handleGetTargetPinStates(GetTargetPinStates& command) {
        return std::make_unique<TargetPinStates>(this->target->getPinStates(command.variantId));
    }

    std::unique_ptr<Response> TargetControllerComponent::handleSetTargetPinState(SetTargetPinState& command) {
        this->target->setPinState(command.pinDescriptor, command.pinState);
        return std::make_unique<Response>();
    }

    std::unique_ptr<TargetStackPointer> TargetControllerComponent::handleGetTargetStackPointer(
        GetTargetStackPointer& command
    ) {
        return std::make_unique<TargetStackPointer>(this->target->getStackPointer());
    }

    std::unique_ptr<TargetProgramCounter> TargetControllerComponent::handleGetTargetProgramCounter(
        GetTargetProgramCounter& command
    ) {
        return std::make_unique<TargetProgramCounter>(this->target->getProgramCounter());
    }

    std::unique_ptr<Response> TargetControllerComponent::handleEnableProgrammingMode(EnableProgrammingMode& command) {
        if (!this->target->programmingModeEnabled()) {
            this->enableProgrammingMode();
        }

        return std::make_unique<Response>();
    }

    std::unique_ptr<Response> TargetControllerComponent::handleDisableProgrammingMode(DisableProgrammingMode& command) {
        if (this->target->programmingModeEnabled()) {
            this->disableProgrammingMode();
        }

        return std::make_unique<Response>();
    }
}

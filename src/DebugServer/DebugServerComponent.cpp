#include "DebugServerComponent.hpp"

#include "src/EventManager/EventManager.hpp"

// Debug server implementations
#include "Gdb/AvrGdb/AvrGdbRsp.hpp"

#include "src/Exceptions/InvalidConfig.hpp"
#include "src/Logger/Logger.hpp"

namespace Bloom::DebugServer
{
    using namespace Bloom::Events;

    DebugServerComponent::DebugServerComponent(const DebugServerConfig& debugServerConfig)
        : debugServerConfig(debugServerConfig)
    {}

    void DebugServerComponent::run() {
        try {
            this->startup();

            Logger::info("DebugServer ready");

            while (this->getThreadState() == ThreadState::READY) {
                this->server->run();
                this->eventListener->dispatchCurrentEvents();
            }

        } catch (const std::exception& exception) {
            Logger::error("DebugServer fatal error: " + std::string(exception.what()));
        }

        this->shutdown();
    }

    std::map<
        std::string,
        std::function<std::unique_ptr<ServerInterface>()>
    > DebugServerComponent::getAvailableServersByName() {
        return std::map<std::string, std::function<std::unique_ptr<ServerInterface>()>> {
            {
                "avr-gdb-rsp",
                [this] () -> std::unique_ptr<ServerInterface> {
                    return std::make_unique<DebugServer::Gdb::AvrGdb::AvrGdbRsp>(
                        this->debugServerConfig,
                        *(this->eventListener.get()),
                        this->interruptEventNotifier
                    );
                }
            },
        };
    }

    void DebugServerComponent::startup() {
        this->setName("DS");
        Logger::info("Starting DebugServer");
        this->blockAllSignals();

        EventManager::registerListener(this->eventListener);
        this->eventListener->setInterruptEventNotifier(&this->interruptEventNotifier);

        // Register event handlers
        this->eventListener->registerCallbackForEventType<Events::ShutdownDebugServer>(
            std::bind(&DebugServerComponent::onShutdownDebugServerEvent, this, std::placeholders::_1)
        );

        static const auto availableServersByName = this->getAvailableServersByName();
        if (!availableServersByName.contains(this->debugServerConfig.name)) {
            throw Exceptions::InvalidConfig(
                "DebugServer \"" + this->debugServerConfig.name + "\" not found."
            );
        }

        this->server = availableServersByName.at(this->debugServerConfig.name)();
        Logger::info("Selected DebugServer: " + this->server->getName());

        this->server->init();
        this->setThreadStateAndEmitEvent(ThreadState::READY);
    }

    void DebugServerComponent::shutdown() {
        if (this->getThreadState() == ThreadState::STOPPED
            || this->getThreadState() == ThreadState::SHUTDOWN_INITIATED
        ) {
            return;
        }

        this->setThreadState(ThreadState::SHUTDOWN_INITIATED);
        Logger::info("Shutting down DebugServer");
        this->server->close();
        this->setThreadStateAndEmitEvent(ThreadState::STOPPED);

        this->eventListener->setInterruptEventNotifier(nullptr);
        EventManager::deregisterListener(this->eventListener->getId());
    }

    void DebugServerComponent::setThreadStateAndEmitEvent(ThreadState state) {
        Thread::setThreadState(state);
        EventManager::triggerEvent(
            std::make_shared<Events::DebugServerThreadStateChanged>(state)
        );
    }

    void DebugServerComponent::onShutdownDebugServerEvent(const Events::ShutdownDebugServer& event) {
        this->shutdown();
    }
}

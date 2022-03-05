#pragma once

#include "TargetInterfaces/Microchip/AVR/AVR8/Avr8DebugInterface.hpp"
#include "TargetInterfaces/Microchip/AVR/AvrIspInterface.hpp"

namespace Bloom
{
    /**
     * A debug tool can be any device that provides access to the connected target. Debug tools are usually connected
     * to the host machine via USB.
     *
     * Each debug tool must implement this interface. Note that target specific driver code should not be placed here.
     * Each target family will expect the debug tool to provide an interface for that particular group of targets.
     * For an example, see the Avr8DebugInterface class and DebugTool::getAvr8DebugInterface(), for the family of AVR
     * 8-bit targets.
     */
    class DebugTool
    {
    public:
        DebugTool() = default;
        virtual ~DebugTool() = default;

        DebugTool(const DebugTool& other) = default;
        DebugTool(DebugTool&& other) = default;

        DebugTool& operator = (const DebugTool& other) = default;
        DebugTool& operator = (DebugTool&& other) = default;

        /**
         * Should establish a connection to the device and prepare it for a debug session.
         */
        virtual void init() = 0;

        /**
         * Should disconnect from the device after performing any tasks required to formally end the debug session.
         */
        virtual void close() = 0;

        virtual std::string getName() = 0;

        virtual std::string getSerialNumber() = 0;

        /**
         * All debug tools that support debugging operations on AVR8 targets must provide an implementation of
         * the Avr8DebugInterface class, via this function.
         *
         * For debug tools that do not support debugging on AVR8 targets, this function should return a nullptr.
         *
         * Note: the caller of this function will not manage the lifetime of the returned Avr8DebugInterface instance.
         *
         * @return
         */
        virtual DebugToolDrivers::TargetInterfaces::Microchip::Avr::Avr8::Avr8DebugInterface* getAvr8DebugInterface() {
            return nullptr;
        }

        /**
         * All debug tools that support interfacing with AVR targets via the ISP interface must provide an
         * implementation of the AvrIspInterface class, via this function.
         *
         * For debug tools that do not support the interface, a nullptr should be returned.
         *
         * Note: the caller of this function will not manage the lifetime of the returned AvrIspInterface instance.
         *
         * @return
         */
        virtual DebugToolDrivers::TargetInterfaces::Microchip::Avr::AvrIspInterface* getAvrIspInterface() {
            return nullptr;
        }

        [[nodiscard]] bool isInitialised() const {
            return this->initialised;
        }

    protected:
        void setInitialised(bool initialised) {
            this->initialised = initialised;
        }

    private:
        bool initialised = false;
    };
}

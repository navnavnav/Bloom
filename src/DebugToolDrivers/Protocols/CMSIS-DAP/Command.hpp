#pragma once

#include <cstdint>
#include <vector>

#include "Response.hpp"

namespace Bloom::DebugToolDrivers::Protocols::CmsisDap
{
    /**
     * CMSIS-DAP command.
     *
     * Casting an instance of this class to an std::vector<unsigned char> will result in the raw buffer of the
     * CMSIS-DAP command, which can then be sent to the device. See the conversion function below.
     */
    class Command
    {
    public:
        /*
         * CMSIS-DAP commands always result in a response. The data structure and contents of the response depends on
         * the command. Because of this, we allow the command class (and any derived classes) to specify the expected
         * response type, via the ExpectedResponseType alias.
         *
         * This alias is used by template functions such as CmsisDapInterface::sendCommandAndWaitForResponse(), to
         * determine which type to use, when constructing and returning a response object. This means we don't have
         * to perform any downcasting with our response objects, and the type of our response objects is known at
         * compile time.
         *
         * For example, consider the AvrResponseCommand - this is a CMSIS-DAP vendor command which requests an AVR
         * response from the device. Upon issuing this command, we expect the device to respond with data in the
         * structure described by the AvrResponse class. So, in the AvrResponseCommand class, we set the
         * ExpectedResponseType alias to AvrResponse. Then, when sending an AvrResponseCommand to the device:
         *
         *   CmsisDapInterface cmsisInterface;
         *   AvrResponseCommand avrResponseCommand;
         *
         *   auto response = cmsisInterface.sendCommandAndWaitForResponse(avrResponseCommand);
         *
         * In the code above, the response object will be an instance of the AvrResponse class, because the
         * CmsisDapInterface::sendCommandAndWaitForResponse() function will use the ExpectedResponseType alias from
         * the command class (AvrResponseCommand::ExpectedResponseType).
         *
         * Effectively, the ExpectedResponseType alias allows us to map CMSIS-DAP command classes to response classes.
         * This mapping is employed by the necessary functions to provide us with response objects of the correct type,
         * improving type safety and type hinting.
         *
         * For more on this, see the implementation of the following template functions:
         *  CmsisDapInterface::sendCommandAndWaitForResponse()
         *  CmsisDapInterface::getResponse()
         */
        using ExpectedResponseType = Response;

        explicit Command(unsigned char commandId): commandId(commandId) {};

        Command() = default;
        virtual ~Command() = default;

        Command(const Command& other) = default;
        Command(Command&& other) = default;

        Command& operator = (const Command& other) = default;
        Command& operator = (Command&& other) = default;

        [[nodiscard]] unsigned char getCommandId() const {
            return this->commandId;
        }

        void setCommandId(unsigned char commandId) {
            this->commandId = commandId;
        }

        [[nodiscard]] virtual std::vector<unsigned char> getData() const {
            return this->data;
        }

        void setData(const std::vector<unsigned char>& data) {
            this->data = data;
        }

        [[nodiscard]] int getCommandSize() const {
            // +1 for the command ID
            return static_cast<int>(1 + this->getData().size());
        }

        [[nodiscard]] std::uint16_t getDataSize() const {
            return static_cast<std::uint16_t>(this->getData().size());
        }

        /**
         * Converts instance of a CMSIS Command to a vector of unsigned char (buffer), for sending
         * to the debug tool.
         *
         * @return
         */
        explicit virtual operator std::vector<unsigned char>() const;

    private:
        unsigned char commandId = 0x00;
        std::vector<unsigned char> data;
    };
}

#include "AtmelIce.hpp"

#include "src/TargetController/Exceptions/DeviceFailure.hpp"
#include "src/TargetController/Exceptions/DeviceInitializationFailure.hpp"

namespace Bloom::DebugToolDrivers
{
    using namespace Protocols::CmsisDap::Edbg::Avr;
    using namespace Bloom::Exceptions;

    void AtmelIce::init() {
        UsbDevice::init();

        // TODO: Move away from hard-coding the CMSIS-DAP/EDBG interface number
        auto& usbHidInterface = this->getEdbgInterface().getUsbHidInterface();
        usbHidInterface.setNumber(0);
        usbHidInterface.setLibUsbDevice(this->libUsbDevice);
        usbHidInterface.setLibUsbDeviceHandle(this->libUsbDeviceHandle);
        usbHidInterface.setVendorId(this->vendorId);
        usbHidInterface.setProductId(this->productId);

        if (!usbHidInterface.isInitialised()) {
            usbHidInterface.detachKernelDriver();
            this->setConfiguration(0);
            usbHidInterface.init();
        }

        /*
         * The Atmel-ICE EDBG/CMSIS-DAP interface doesn't operate properly when sending commands too quickly.
         *
         * Because of this, we have to enforce a minimum time gap between commands. See comment
         * in CmsisDapInterface class declaration for more info.
         */
        this->getEdbgInterface().setMinimumCommandTimeGap(std::chrono::milliseconds(35));

        // We don't need to claim the CMSISDAP interface here as the HIDAPI will have already done so.
        if (!this->sessionStarted) {
            this->startSession();
        }

        this->edbgAvr8Interface = std::make_unique<EdbgAvr8Interface>(this->edbgInterface);
        this->setInitialised(true);
    }

    void AtmelIce::close() {
        if (this->sessionStarted) {
            this->endSession();
        }

        this->getEdbgInterface().getUsbHidInterface().close();
        UsbDevice::close();
    }

    std::string AtmelIce::getSerialNumber() {
        using namespace CommandFrames::Discovery;
        using ResponseFrames::Discovery::ResponseId;

        auto response = this->getEdbgInterface().sendAvrCommandFrameAndWaitForResponseFrame(
            Query(QueryContext::SERIAL_NUMBER)
        );

        if (response.getResponseId() != ResponseId::OK) {
            throw DeviceInitializationFailure(
                "Failed to fetch serial number from device - invalid Discovery Protocol response ID."
            );
        }

        auto data = response.getPayloadData();
        return std::string(data.begin(), data.end());
    }

    void AtmelIce::startSession() {
        using namespace CommandFrames::HouseKeeping;
        using ResponseFrames::HouseKeeping::ResponseId;

        auto response = this->getEdbgInterface().sendAvrCommandFrameAndWaitForResponseFrame(
            StartSession()
        );

        if (response.getResponseId() == ResponseId::FAILED) {
            // Failed response returned!
            throw DeviceInitializationFailure("Failed to start session with Atmel-ICE!");
        }

        this->sessionStarted = true;
    }

    void AtmelIce::endSession() {
        using namespace CommandFrames::HouseKeeping;
        using ResponseFrames::HouseKeeping::ResponseId;

        auto response = this->getEdbgInterface().sendAvrCommandFrameAndWaitForResponseFrame(
            EndSession()
        );

        if (response.getResponseId() == ResponseId::FAILED) {
            // Failed response returned!
            throw DeviceFailure("Failed to end session with Atmel-ICE!");
        }

        this->sessionStarted = false;
    }
}

#include "FocusedRegionItem.hpp"

#include <QFile>

#include "src/Helpers/Paths.hpp"
#include "src/Exceptions/Exception.hpp"
#include "src/Insight/UserInterfaces/InsightWindow/UiLoader.hpp"

namespace Bloom::Widgets
{
    using Targets::TargetMemoryAddressRange;

    FocusedRegionItem::FocusedRegionItem(
        const FocusedMemoryRegion& region,
        const Targets::TargetMemoryDescriptor& memoryDescriptor,
        QWidget* parent
    )
        : memoryRegion(region), RegionItem(region, memoryDescriptor, parent)
    {
        auto formUiFile = QFile(
            QString::fromStdString(Paths::compiledResourcesPath()
                + "/src/Insight/UserInterfaces/InsightWindow/Widgets/TargetMemoryInspectionPane"
                    + "/MemoryRegionManager/UiFiles/FocusedMemoryRegionForm.ui"
            )
        );

        if (!formUiFile.open(QFile::ReadOnly)) {
            throw Bloom::Exceptions::Exception("Failed to open focused region item form UI file");
        }

        auto uiLoader = UiLoader(this);
        this->formWidget = uiLoader.load(&formUiFile, this);

        this->initFormInputs();
    }

    void FocusedRegionItem::applyChanges() {
        this->memoryRegion.name = this->nameInput->text();

        const auto inputAddressRange = TargetMemoryAddressRange(
            this->startAddressInput->text().toUInt(nullptr, 16),
            this->endAddressInput->text().toUInt(nullptr, 16)
        );
        this->memoryRegion.addressRangeInputType = this->getSelectedAddressInputType();
        this->memoryRegion.addressRange =
            this->memoryRegion.addressRangeInputType == MemoryRegionAddressInputType::RELATIVE ?
                this->convertRelativeToAbsoluteAddressRange(inputAddressRange) : inputAddressRange;

        auto selectedDataTypeOptionName = this->dataTypeInput->currentData().toString();
        if (FocusedRegionItem::dataTypeOptionsByName.contains(selectedDataTypeOptionName)) {
            this->memoryRegion.dataType = FocusedRegionItem::dataTypeOptionsByName.at(
                selectedDataTypeOptionName
            ).dataType;
        }

        auto selectedEndiannessOptionName = this->endiannessInput->currentData().toString();
        if (FocusedRegionItem::endiannessOptionsByName.contains(selectedEndiannessOptionName)) {
            this->memoryRegion.endianness = FocusedRegionItem::endiannessOptionsByName.at(
                selectedEndiannessOptionName
            ).endianness;
        }
    }

    void FocusedRegionItem::initFormInputs() {
        RegionItem::initFormInputs();
        const auto& region = this->memoryRegion;

        this->dataTypeInput = this->formWidget->findChild<QComboBox*>("data-type-input");
        this->endiannessInput = this->formWidget->findChild<QComboBox*>("endianness-input");

        for (const auto& [optionName, option] : FocusedRegionItem::dataTypeOptionsByName) {
            this->dataTypeInput->addItem(option.text, optionName);
        }

        for (const auto& [optionName, option] : FocusedRegionItem::endiannessOptionsByName) {
            this->endiannessInput->addItem(option.text, optionName);
        }

        switch (region.dataType) {
            case MemoryRegionDataType::UNSIGNED_INTEGER: {
                this->dataTypeInput->setCurrentText(
                    FocusedRegionItem::dataTypeOptionsByName.at("unsigned_integer").text
                );
                break;
            }
            case MemoryRegionDataType::SIGNED_INTEGER: {
                this->dataTypeInput->setCurrentText(
                    FocusedRegionItem::dataTypeOptionsByName.at("signed_integer").text
                );
                break;
            }
            case MemoryRegionDataType::ASCII_STRING: {
                this->dataTypeInput->setCurrentText(FocusedRegionItem::dataTypeOptionsByName.at("ascii").text);
                break;
            }
            default: {
                this->dataTypeInput->setCurrentText(FocusedRegionItem::dataTypeOptionsByName.at("other").text);
            }
        }

        switch (region.endianness) {
            case Targets::TargetMemoryEndianness::LITTLE: {
                this->endiannessInput->setCurrentText(FocusedRegionItem::endiannessOptionsByName.at("little").text);
                break;
            }
            case Targets::TargetMemoryEndianness::BIG: {
                this->endiannessInput->setCurrentText(FocusedRegionItem::endiannessOptionsByName.at("big").text);
                break;
            }
        }
    }
}

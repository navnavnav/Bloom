#include "ByteItem.hpp"

#include <QPainter>

namespace Bloom::Widgets
{
    ByteItem::ByteItem(
        std::size_t byteIndex,
        std::uint32_t address,
        std::optional<std::uint32_t>& currentStackPointer,
        std::optional<ByteItem*>& hoveredByteItem,
        std::optional<AnnotationItem*>& hoveredAnnotationItem,
        std::set<std::uint32_t>& highlightedAddresses,
        const HexViewerWidgetSettings& settings
    )
        : QGraphicsItem(nullptr)
        , byteIndex(byteIndex)
        , address(address)
        , currentStackPointer(currentStackPointer)
        , hoveredByteItem(hoveredByteItem)
        , hoveredAnnotationItem(hoveredAnnotationItem)
        , highlightedAddresses(highlightedAddresses)
        , settings(settings)
    {
        this->setCacheMode(
            QGraphicsItem::CacheMode::ItemCoordinateCache,
            QSize(ByteItem::WIDTH, ByteItem::HEIGHT)
        );
        this->setAcceptHoverEvents(true);

        this->addressHex = "0x" + QString::number(this->address, 16).rightJustified(
            8,
            '0'
        ).toUpper();
        this->relativeAddressHex = "0x" + QString::number(this->byteIndex, 16).rightJustified(
            8,
            '0'
        ).toUpper();

        this->setSelected(false);
    }

    void ByteItem::setValue(unsigned char value) {
        this->valueChanged = this->valueInitialised && this->value != value;

        this->value = value;
        this->hexValue = QString::number(this->value, 16).rightJustified(2, '0').toUpper();
        this->asciiValue = (this->value >= 32 && this->value <= 126)
            ? std::optional("'" + QString(QChar(this->value)) + "'") : std::nullopt;

        this->valueInitialised = this->excludedMemoryRegion == nullptr;
        this->update();
    }

    void ByteItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        painter->setRenderHints(
            QPainter::RenderHint::Antialiasing | QPainter::RenderHint::SmoothPixmapTransform,
            true
        );
        painter->setPen(Qt::PenStyle::NoPen);

        // TODO: This code could do with some tidying. It's getting quite messy.

        static const auto widgetRect = this->boundingRect();
        static const auto standardTextColor = QColor(0xAF, 0xB1, 0xB3);
        static const auto valueChangedTextColor = QColor(0x54, 0x7F, 0xBA);
        static auto font = QFont("'Ubuntu', sans-serif");

        static const auto highlightedBackgroundColor = QColor(0x3C, 0x59, 0x5C, 255);
        static const auto focusedRegionBackgroundColor = QColor(0x44, 0x44, 0x41, 255);
        static const auto stackMemoryBackgroundColor = QColor(0x67, 0x57, 0x20, 210);
        static const auto hoveredBackgroundColor = QColor(0x8E, 0x8B, 0x83, 70);
        static const auto hoveredNeighbourBackgroundColor = QColor(0x8E, 0x8B, 0x83, 30);
        static const auto hoveredAnnotationBackgroundColor = QColor(0x8E, 0x8B, 0x83, 50);

        const auto isEnabled = this->isEnabled();

        const auto highlightingEnabled = !this->highlightedAddresses.empty();
        const auto highlightedByte = highlightingEnabled && this->highlightedAddresses.contains(this->address);

        auto textColor = standardTextColor;
        auto asciiTextColor = QColor(0xA7, 0x77, 0x26);

        auto backgroundColor = std::optional<QColor>();

        font.setPixelSize(11);
        painter->setFont(font);

        if (highlightedByte) {
            backgroundColor = highlightedBackgroundColor;
            asciiTextColor = standardTextColor;

        } else if (this->settings.highlightStackMemory && this->currentStackPointer.has_value()
            && this->address > this->currentStackPointer
        ) {
            // This byte is within the stack memory
            backgroundColor = stackMemoryBackgroundColor;
            asciiTextColor = standardTextColor;

        } else if (this->settings.highlightFocusedMemory && this->focusedMemoryRegion != nullptr) {
            // This byte is within a focused region
            backgroundColor = focusedRegionBackgroundColor;
        }

        const auto* hoveredByteItem = this->hoveredByteItem.value_or(nullptr);
        const auto* hoveredAnnotationItem = this->hoveredAnnotationItem.value_or(nullptr);
        if (hoveredByteItem != nullptr) {
            if (hoveredByteItem == this) {
                if (backgroundColor.has_value()) {
                    backgroundColor->setAlpha(255);

                } else {
                    backgroundColor = hoveredBackgroundColor;
                }

            } else if (this->settings.highlightHoveredRowAndCol
                && (
                    hoveredByteItem->currentColumnIndex == this->currentColumnIndex
                    || hoveredByteItem->currentRowIndex == this->currentRowIndex
                )
            ) {
                if (backgroundColor.has_value()) {
                    backgroundColor->setAlpha(220);

                } else {
                    backgroundColor = hoveredNeighbourBackgroundColor;
                }
            }

        } else if (
            !this->settings.highlightFocusedMemory
            && hoveredAnnotationItem != nullptr
            && this->address >= hoveredAnnotationItem->startAddress
            && this->address <= hoveredAnnotationItem->endAddress
        ) {
            backgroundColor = hoveredAnnotationBackgroundColor;
        }

        if (backgroundColor.has_value()) {
            if (!isEnabled || (highlightingEnabled && !highlightedByte)) {
                backgroundColor->setAlpha(100);
            }

            painter->setBrush(backgroundColor.value());
            painter->drawRect(widgetRect);
        }

        if (this->valueInitialised && this->excludedMemoryRegion == nullptr) {
            if (this->settings.displayAsciiValues && this->asciiValue.has_value()) {
                if (!isEnabled || (highlightingEnabled && !highlightedByte)) {
                    asciiTextColor.setAlpha(100);
                }

                painter->setPen(asciiTextColor);
                painter->drawText(widgetRect, Qt::AlignCenter, this->asciiValue.value());

            } else {
                if (!isEnabled || (highlightingEnabled && !highlightedByte) || this->settings.displayAsciiValues) {
                    textColor.setAlpha(100);
                }

                painter->setPen(textColor);
                painter->drawText(widgetRect, Qt::AlignCenter, this->hexValue);
                return;
            }

        } else {
            textColor.setAlpha(100);
            painter->setPen(textColor);

            static const auto placeholderString = QString("??");
            painter->drawText(widgetRect, Qt::AlignCenter, placeholderString);
        }
    }
}

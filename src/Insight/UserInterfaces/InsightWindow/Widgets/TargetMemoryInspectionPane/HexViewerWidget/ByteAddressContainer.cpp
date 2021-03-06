#include "ByteAddressContainer.hpp"

namespace Bloom::Widgets
{
    void ByteAddressContainer::adjustAddressLabels(
        const std::map<std::size_t, std::vector<ByteItem*>>& byteItemsByRowIndex
    ) {
        static constexpr int leftMargin = 10;
        const auto newRowCount = byteItemsByRowIndex.size();
        const auto layoutItemMaxIndex = static_cast<int>(this->addressItemsByRowIndex.size() - 1);

        for (const auto& mappingPair : byteItemsByRowIndex) {
            const auto rowIndex = static_cast<std::size_t>(mappingPair.first);
            const auto& byteItems = mappingPair.second;

            if (byteItems.empty()) {
                continue;
            }

            ByteAddressItem* addressLabel = nullptr;
            if (static_cast<int>(rowIndex) > layoutItemMaxIndex) {
                addressLabel = new ByteAddressItem(this);
                this->addressItemsByRowIndex.insert(std::pair(rowIndex, addressLabel));

            } else {
                addressLabel = this->addressItemsByRowIndex.at(rowIndex);
            }

            const auto& firstByteItem = byteItems.front();
            addressLabel->setAddressHex(firstByteItem->relativeAddressHex);
            addressLabel->setPos(
                leftMargin,
                firstByteItem->pos().y()
            );
        }

        // Delete any address items we no longer need
        const auto addressItemCount = this->addressItemsByRowIndex.size();

        if (newRowCount > 0 && newRowCount < addressItemCount) {
            for (auto i = (addressItemCount - 1); i >= newRowCount; i--) {
                delete this->addressItemsByRowIndex.at(i);
                this->addressItemsByRowIndex.erase(i);
            }
        }

        this->update();
    }

    void ByteAddressContainer::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
        static const auto backgroundColor = QColor(0x35, 0x36, 0x33);
        static const auto borderColor = QColor(0x41, 0x42, 0x3F);

        painter->setPen(Qt::PenStyle::NoPen);
        painter->setBrush(backgroundColor);
        painter->drawRect(
            0,
            0,
            ByteAddressContainer::WIDTH,
            static_cast<int>(this->boundingRect().height())
        );

        painter->setPen(borderColor);
        painter->drawLine(
            ByteAddressContainer::WIDTH - 1,
            0,
            ByteAddressContainer::WIDTH - 1,
            static_cast<int>(this->boundingRect().height())
        );
    }
}

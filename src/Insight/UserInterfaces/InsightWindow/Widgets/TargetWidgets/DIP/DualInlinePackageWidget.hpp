#pragma once

#include <QWidget>
#include <vector>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "../TargetPackageWidget.hpp"

#include "PinWidget.hpp"
#include "BodyWidget.hpp"
#include "src/Targets/TargetVariant.hpp"

namespace Bloom::Widgets::InsightTargetWidgets::Dip
{
    /**
     * The DualInlinePackageWidget implements a custom Qt widget for the Dual-inline package target variants.
     */
    class DualInlinePackageWidget: public TargetPackageWidget
    {
        Q_OBJECT

    public:
        DualInlinePackageWidget(
            const Targets::TargetVariant& targetVariant,
            InsightWorker& insightWorker,
            QWidget* parent
        );

    private:
        QVBoxLayout* layout = nullptr;
        QHBoxLayout* topPinLayout = nullptr;
        QHBoxLayout* bottomPinLayout = nullptr;
        BodyWidget* bodyWidget = nullptr;
    };
}

#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <vector>
#include <QEvent>
#include <QFile>

#include "../../../InsightWindow.hpp"
#include "QuadFlatPackageWidget.hpp"
#include "src/Helpers/Paths.hpp"
#include "PinWidget.hpp"
#include "BodyWidget.hpp"

using namespace Bloom::Widgets::InsightTargetWidgets::Qfp;
using namespace Bloom::Targets;

QuadFlatPackageWidget::QuadFlatPackageWidget(
    const TargetVariant& targetVariant,
    QObject* insightWindowObj,
    QWidget* parent
): TargetPackageWidget(targetVariant, insightWindowObj, parent) {
    assert((targetVariant.pinDescriptorsByNumber.size() % 4) == 0);

    auto stylesheetFile = QFile(QString::fromStdString(
            Paths::compiledResourcesPath()
            + "/src/Insight/UserInterfaces/InsightWindow/Widgets/TargetWidgets//QFP/Stylesheets/QuadFlatPackage.qss"
        )
    );
    stylesheetFile.open(QFile::ReadOnly);
    this->setStyleSheet(stylesheetFile.readAll());

    this->pinWidgets.reserve(targetVariant.pinDescriptorsByNumber.size());
    this->layout = new QVBoxLayout();
    this->layout->setSpacing(0);
    this->layout->setMargin(0);

    this->horizontalLayout = new QHBoxLayout();
    this->horizontalLayout->setSpacing(0);
    this->horizontalLayout->setDirection(QBoxLayout::Direction::LeftToRight);

    this->topPinLayout = new QHBoxLayout();
    this->topPinLayout->setSpacing(QuadFlatPackageWidget::PIN_WIDGET_SPACING);
    this->topPinLayout->setDirection(QBoxLayout::Direction::RightToLeft);

    this->rightPinLayout = new QVBoxLayout();
    this->rightPinLayout->setSpacing(QuadFlatPackageWidget::PIN_WIDGET_SPACING);
    this->rightPinLayout->setDirection(QBoxLayout::Direction::BottomToTop);

    this->bottomPinLayout = new QHBoxLayout();
    this->bottomPinLayout->setSpacing(QuadFlatPackageWidget::PIN_WIDGET_SPACING);
    this->bottomPinLayout->setDirection(QBoxLayout::Direction::LeftToRight);

    this->leftPinLayout = new QVBoxLayout();
    this->leftPinLayout->setSpacing(QuadFlatPackageWidget::PIN_WIDGET_SPACING);
    this->leftPinLayout->setDirection(QBoxLayout::Direction::TopToBottom);

    auto insightWindow = qobject_cast<InsightWindow*>(insightWindowObj);
    assert(insightWindow != nullptr);

    auto pinCountPerLayout = (targetVariant.pinDescriptorsByNumber.size() / 4);
    for (const auto& [targetPinNumber, targetPinDescriptor]: targetVariant.pinDescriptorsByNumber) {
        auto pinWidget = new PinWidget(this, targetPinDescriptor, targetVariant);
        this->pinWidgets.push_back(pinWidget);

        if (targetPinNumber <= pinCountPerLayout) {
            this->leftPinLayout->addWidget(pinWidget, 0, Qt::AlignmentFlag::AlignRight);

        } else if (targetPinNumber > pinCountPerLayout && targetPinNumber <= (pinCountPerLayout * 2)) {
            this->bottomPinLayout->addWidget(pinWidget, 0, Qt::AlignmentFlag::AlignTop);

        } else if (targetPinNumber > (pinCountPerLayout * 2) && targetPinNumber <= (pinCountPerLayout * 3)) {
            this->rightPinLayout->addWidget(pinWidget, 0, Qt::AlignmentFlag::AlignLeft);

        } else if (targetPinNumber > (pinCountPerLayout * 3) && targetPinNumber <= (pinCountPerLayout * 4)) {
            this->topPinLayout->addWidget(pinWidget, 0, Qt::AlignmentFlag::AlignBottom);
        }

        connect(pinWidget, &TargetPinWidget::toggleIoState, insightWindow, &InsightWindow::togglePinIoState);
    }

    this->bodyWidget = new BodyWidget(this);
    this->layout->addLayout(this->topPinLayout);
    this->horizontalLayout->addLayout(this->leftPinLayout);
    this->horizontalLayout->addWidget(this->bodyWidget);
    this->horizontalLayout->addLayout(this->rightPinLayout);
    this->layout->addLayout(this->horizontalLayout);
    this->layout->addLayout(this->bottomPinLayout);
    this->setLayout(this->layout);

    auto insightWindowWidget = this->window();
    assert(insightWindowWidget != nullptr);

    insightWindowWidget->setMinimumHeight(
        std::max(
            500,
            static_cast<int>((PinWidget::MAXIMUM_HORIZONTAL_HEIGHT * pinCountPerLayout)
                + (PinWidget::MAXIMUM_VERTICAL_HEIGHT * 2)) + 300
        )
    );

    insightWindowWidget->setMinimumWidth(
        std::max(
            1000,
            static_cast<int>((PinWidget::MAXIMUM_VERTICAL_WIDTH * pinCountPerLayout)
                + (PinWidget::MAXIMUM_VERTICAL_WIDTH * 2)) + 300
        )
    );
}

void QuadFlatPackageWidget::paintEvent(QPaintEvent* event) {
    auto painter = QPainter(this);
    this->drawWidget(painter);
}

void QuadFlatPackageWidget::drawWidget(QPainter& painter) {
    auto parentWidget = this->parentWidget();
    assert(parentWidget != nullptr);

    auto parentContainerHeight = parentWidget->height();
    auto parentContainerWidth = parentWidget->width();

    auto verticalPinWidgetHeight = PinWidget::MAXIMUM_VERTICAL_HEIGHT;
    auto verticalPinWidgetWidth = PinWidget::MAXIMUM_VERTICAL_WIDTH;
    auto horizontalPinWidgetHeight = PinWidget::MAXIMUM_HORIZONTAL_HEIGHT;
    auto horizontalPinWidgetWidth = PinWidget::MAXIMUM_HORIZONTAL_WIDTH;

    auto pinCountPerLayout = static_cast<int>(this->pinWidgets.size() / 4);

    /*
     * Horizontal layouts are the right and left pin layouts - the ones that hold horizontal pin widgets.
     * The bottom and top layouts are vertical layouts, as they hold the vertical pin widgets.
     */
    auto horizontalLayoutHeight = ((horizontalPinWidgetHeight + QuadFlatPackageWidget::PIN_WIDGET_SPACING) * pinCountPerLayout
        + QuadFlatPackageWidget::PIN_WIDGET_LAYOUT_PADDING - QuadFlatPackageWidget::PIN_WIDGET_SPACING);

    auto verticalLayoutWidth = ((verticalPinWidgetWidth + QuadFlatPackageWidget::PIN_WIDGET_SPACING) * pinCountPerLayout
        + QuadFlatPackageWidget::PIN_WIDGET_LAYOUT_PADDING - QuadFlatPackageWidget::PIN_WIDGET_SPACING);

    auto containerWidth = verticalLayoutWidth + (horizontalPinWidgetWidth * 2);

    this->topPinLayout->setGeometry(QRect(
        horizontalPinWidgetWidth,
        0,
        verticalLayoutWidth,
        verticalPinWidgetHeight
    ));

    this->horizontalLayout->setGeometry(QRect(
        0,
        verticalPinWidgetHeight,
        containerWidth,
        horizontalLayoutHeight
    ));

    this->leftPinLayout->setGeometry(QRect(
        0,
        verticalPinWidgetHeight,
        horizontalPinWidgetWidth,
        horizontalLayoutHeight
    ));

    this->bodyWidget->setGeometry(QRect(
        horizontalPinWidgetWidth,
        verticalPinWidgetHeight,
        horizontalLayoutHeight,
        horizontalLayoutHeight
    ));

    this->rightPinLayout->setGeometry(QRect(
        horizontalLayoutHeight + horizontalPinWidgetWidth,
        verticalPinWidgetHeight,
        horizontalPinWidgetWidth,
        horizontalLayoutHeight
    ));

    this->bottomPinLayout->setGeometry(QRect(
        horizontalPinWidgetWidth,
        verticalPinWidgetHeight + horizontalLayoutHeight,
        verticalLayoutWidth,
        verticalPinWidgetHeight
    ));

    auto pinWidgetLayoutMargin = QuadFlatPackageWidget::PIN_WIDGET_LAYOUT_PADDING / 2;

    this->topPinLayout->setContentsMargins(
        pinWidgetLayoutMargin,
        0,
        pinWidgetLayoutMargin,
        0
    );

    this->bottomPinLayout->setContentsMargins(
        pinWidgetLayoutMargin,
        0,
        pinWidgetLayoutMargin,
        0
    );

    this->leftPinLayout->setContentsMargins(
        0,
        pinWidgetLayoutMargin,
        0,
        pinWidgetLayoutMargin
    );

    this->rightPinLayout->setContentsMargins(
        0,
        pinWidgetLayoutMargin,
        0,
        pinWidgetLayoutMargin
    );

    auto containerHeight = horizontalLayoutHeight + (verticalPinWidgetHeight * 2);
    this->setGeometry(
        (parentContainerWidth / 2) - (containerWidth / 2),
        (parentContainerHeight / 2) - (containerHeight / 2),
        containerWidth,
        containerHeight
    );
}
#include <QPainter>
#include <QEvent>

#include "PinBodyWidget.hpp"

using namespace Bloom::Widgets::InsightTargetWidgets::Qfp;
using namespace Bloom::Targets;

void PinBodyWidget::paintEvent(QPaintEvent* event) {
    auto painter = QPainter(this);
    this->drawWidget(painter);
}

void PinBodyWidget::drawWidget(QPainter& painter) {
    painter.setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::SmoothPixmapTransform, true);
    auto pinWidth = this->isVertical ? PinBodyWidget::WIDTH : PinBodyWidget::HEIGHT;
    auto pinHeight = this->isVertical ? PinBodyWidget::HEIGHT : PinBodyWidget::WIDTH;
    this->setFixedSize(pinWidth, pinHeight);

    auto pinColor = this->getBodyColor();

    painter.setPen(Qt::PenStyle::NoPen);
    painter.setBrush(pinColor);

    painter.drawRect(
        0,
        0,
        pinWidth,
        pinHeight
    );
}
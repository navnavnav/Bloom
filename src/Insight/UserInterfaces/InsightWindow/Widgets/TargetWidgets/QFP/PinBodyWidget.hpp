#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <utility>

#include "../TargetPinBodyWidget.hpp"
#include "src/Targets/TargetPinDescriptor.hpp"

namespace Bloom::Widgets::InsightTargetWidgets::Qfp
{
    class PinBodyWidget: public TargetPinBodyWidget
    {
    private:
        bool isVertical = false;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void drawWidget(QPainter& painter);

    public:
        static const int WIDTH = 30;
        static const int HEIGHT = 40;

        PinBodyWidget(QWidget* parent, Targets::TargetPinDescriptor pinDescriptor, bool isVertical):
        TargetPinBodyWidget(parent, std::move(pinDescriptor)), isVertical(isVertical) {
            this->setObjectName("target-pin-body");

            if (isVertical) {
                this->setFixedSize(PinBodyWidget::WIDTH, PinBodyWidget::HEIGHT);

            } else {
                this->setFixedSize(PinBodyWidget::HEIGHT, PinBodyWidget::WIDTH);
            }
        }
    };
}
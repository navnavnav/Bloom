#pragma once

#include <QSize>

#include "Label.hpp"

namespace Bloom::Widgets
{
    class RotatableLabel: public Label
    {
        Q_OBJECT

    public:
        RotatableLabel(const QString& text, QWidget* parent)
            : Label(text, parent)
        {};

        RotatableLabel(int angleDegrees, const QString& text, QWidget* parent)
            : Label(text, parent)
            , angle(angleDegrees)
        {};

        void setAngle(int angleDegrees) {
            this->angle = angleDegrees;
        }

    protected:
        void paintEvent(QPaintEvent* event) override;

        [[nodiscard]] QSize sizeHint() const override {
            return this->getContainerSize();
        }

        [[nodiscard]] QSize minimumSizeHint() const override {
            return this->getContainerSize();
        }

    private:
        int angle = 90;

        [[nodiscard]] QSize getContainerSize() const;
    };
}

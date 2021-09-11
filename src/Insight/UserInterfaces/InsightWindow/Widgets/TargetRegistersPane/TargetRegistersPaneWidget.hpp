#pragma once

#include <QWidget>
#include <QLineEdit>
#include <set>
#include <QSize>
#include <QString>
#include <QEvent>
#include <optional>

#include "ItemWidget.hpp"
#include "../SvgToolButton.hpp"
#include "src/Insight/InsightWorker/InsightWorker.hpp"
#include "src/Targets/TargetState.hpp"
#include "src/Targets/TargetDescriptor.hpp"

namespace Bloom::Widgets
{
    class RegisterGroupWidget;

    class TargetRegistersPaneWidget: public QWidget
    {
    Q_OBJECT
    private:
        const Targets::TargetDescriptor& targetDescriptor;
        InsightWorker& insightWorker;

        QWidget* parent = nullptr;
        QWidget* container = nullptr;

        QWidget* toolBar = nullptr;
        SvgToolButton* collapseAllButton = nullptr;
        SvgToolButton* expandAllButton = nullptr;

        QLineEdit* searchInput = nullptr;
        QWidget* itemContainer = nullptr;

        ItemWidget* selectedItemWidget = nullptr;

        std::set<RegisterGroupWidget*> registerGroupWidgets;
        Targets::TargetRegisterDescriptors renderedDescriptors;

        Targets::TargetState targetState = Targets::TargetState::UNKNOWN;

    private slots:
        void onTargetStateChanged(Targets::TargetState newState);
        void onRegistersRead(const Targets::TargetRegisters& registers);
        void onRegistersWritten(const Bloom::Targets::TargetRegisterDescriptors& descriptors);

    protected:
        void resizeEvent(QResizeEvent* event) override;

        virtual void postActivate();
        virtual void postDeactivate();

    public:
        bool activated = false;

        TargetRegistersPaneWidget(
            const Targets::TargetDescriptor& targetDescriptor,
            InsightWorker& insightWorker,
            QWidget *parent
        );

        [[nodiscard]] QSize minimumSizeHint() const override {
            return this->parent->size();
        }

        [[nodiscard]] QSize sizeHint() const override {
            return this->minimumSizeHint();
        }

        void filterRegisters(const QString& keyword);
        void collapseAllRegisterGroups();
        void expandAllRegisterGroups();

        void refreshRegisterValues(std::optional<std::function<void(void)>> callback = std::nullopt);
        void clearInlineValues();

        void activate();
        void deactivate();

    public slots:
        void onItemSelectionChange(ItemWidget* newlySelectedWidget);
    };
}

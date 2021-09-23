#include "TargetRegisterInspectorWindow.hpp"

#include <QVBoxLayout>
#include <QMargins>
#include <QTableWidget>
#include <QScrollBar>
#include <set>
#include <QDesktopServices>

#include "../../UiLoader.hpp"

#include "src/Helpers/Paths.hpp"
#include "src/Helpers/DateTime.hpp"
#include "src/Exceptions/Exception.hpp"

#include "src/Insight/InsightWorker/Tasks/ReadTargetRegisters.hpp"
#include "src/Insight/InsightWorker/Tasks/WriteTargetRegister.hpp"

using namespace Bloom::Widgets;
using namespace Bloom::Exceptions;

using Bloom::Targets::TargetRegisterDescriptor;
using Bloom::Targets::TargetRegisterDescriptors;
using Bloom::Targets::TargetRegisterType;
using Bloom::Targets::TargetState;

TargetRegisterInspectorWindow::TargetRegisterInspectorWindow(
    const Targets::TargetRegisterDescriptor& registerDescriptor,
    InsightWorker& insightWorker,
    TargetState currentTargetState,
    std::optional<Targets::TargetMemoryBuffer> registerValue
):
registerDescriptor(registerDescriptor),
insightWorker(insightWorker),
registerValue(registerValue.value_or(Targets::TargetMemoryBuffer(registerDescriptor.size, 0))) {
    auto registerName = QString::fromStdString(this->registerDescriptor.name.value()).toUpper();
    this->setObjectName("target-register-inspector-window");
    this->setWindowTitle("Inspect Register");

    auto windowUiFile = QFile(
        QString::fromStdString(Paths::compiledResourcesPath()
            + "/src/Insight/UserInterfaces/InsightWindow/Widgets/TargetRegisterInspector/UiFiles/TargetRegisterInspectorWindow.ui"
        )
    );

    auto windowStylesheet = QFile(QString::fromStdString(
        Paths::compiledResourcesPath()
            + "/src/Insight/UserInterfaces/InsightWindow/Widgets/TargetRegisterInspector/Stylesheets/TargetRegisterInspectorWindow.qss"
        )
    );

    if (!windowUiFile.open(QFile::ReadOnly)) {
        throw Exception("Failed to open TargetRegisterInspectorWindow UI file");
    }

    if (!windowStylesheet.open(QFile::ReadOnly)) {
        throw Exception("Failed to open TargetRegisterInspectorWindow stylesheet file");
    }

    auto windowSize = QSize(
        840,
        static_cast<int>(440 + ((BitsetWidget::HEIGHT + 20)
            * std::ceil(static_cast<float>(this->registerValue.size()) / 2)))
    );
    auto containerMargins = QMargins(15, 30, 15, 10);

    this->setStyleSheet(windowStylesheet.readAll());
    this->setFixedSize(windowSize);

    auto uiLoader = UiLoader(this);
    this->container = uiLoader.load(&windowUiFile, this);

    this->container->setFixedSize(this->size());
    this->container->setContentsMargins(containerMargins);

    this->registerNameLabel = this->container->findChild<QLabel*>("register-name");
    this->registerDescriptionLabel = this->container->findChild<QLabel*>("register-description");
    this->contentContainer = this->container->findChild<QWidget*>("content-container");
    this->registerValueContainer = this->container->findChild<QWidget*>("register-value-container");
    this->registerValueTextInput = this->container->findChild<QLineEdit*>("register-value-text-input");
    this->registerValueBitsetWidgetContainer = this->container->findChild<QWidget*>("register-value-bitset-widget-container");
    this->refreshValueButton = this->container->findChild<QPushButton*>("refresh-value-btn");
    this->applyButton = this->container->findChild<QPushButton*>("apply-btn");
    this->helpButton = this->container->findChild<QPushButton*>("help-btn");
    this->closeButton = this->container->findChild<QPushButton*>("close-btn");

    this->registerNameLabel->setText(registerName);

    if (this->registerDescriptor.description.has_value()) {
        this->registerDescriptionLabel->setText(QString::fromStdString(this->registerDescriptor.description.value()));
        this->registerDescriptionLabel->setVisible(true);
    }

    this->registerHistoryWidget = new RegisterHistoryWidget(
        this->registerDescriptor,
        this->registerValue,
        insightWorker,
        this->contentContainer
    );

    auto contentLayout = this->container->findChild<QHBoxLayout*>("content-layout");
    contentLayout->insertWidget(0, this->registerHistoryWidget, 0, Qt::AlignmentFlag::AlignTop);

    auto registerDetailsContainer = this->container->findChild<QWidget*>("register-details-container");
    auto registerValueContainer = this->container->findChild<QWidget*>("register-value-container");
    registerValueContainer->setContentsMargins(15, 15, 15, 15);
    registerDetailsContainer->setContentsMargins(15, 15, 15, 15);

    auto registerDetailsNameInput = registerDetailsContainer->findChild<QLineEdit*>("register-details-name-input");
    auto registerDetailsSizeInput = registerDetailsContainer->findChild<QLineEdit*>("register-details-size-input");
    auto registerDetailsStartAddressInput = registerDetailsContainer->findChild<QLineEdit*>(
        "register-details-start-address-input"
    );
    registerDetailsNameInput->setText(registerName);
    registerDetailsSizeInput->setText(QString::number(this->registerDescriptor.size));
    registerDetailsStartAddressInput->setText(
        "0x" + QString::number(this->registerDescriptor.startAddress.value(), 16).toUpper()
    );

    this->registerValueTextInput->setFixedWidth(BitsetWidget::WIDTH * 2);

    auto registerBitsetWidgetLayout = this->registerValueBitsetWidgetContainer->findChild<QVBoxLayout*>(
        "register-value-bitset-widget-layout"
    );

    /*
     * Each row of the BitsetWidget container should hold two BitsetWidgets. So we have a horizontal layout nested
     * within a vertical layout.
     */
    auto bitsetSingleHorizontalLayout = new QHBoxLayout();
    bitsetSingleHorizontalLayout->setSpacing(BitWidget::SPACING);
    bitsetSingleHorizontalLayout->setContentsMargins(0, 0, 0, 0);

    // The register value will be in MSB, which is OK for us as we present the bit widgets in MSB.
    auto byteNumber = static_cast<int>(this->registerValue.size() - 1);
    for (std::uint32_t registerByteIndex = 0; registerByteIndex < this->registerValue.size(); registerByteIndex++) {
        auto bitsetWidget = new BitsetWidget(
            byteNumber,
            this->registerValue.at(registerByteIndex),
            !this->registerDescriptor.writable,
            this
        );

        bitsetSingleHorizontalLayout->addWidget(bitsetWidget, 0, Qt::AlignmentFlag::AlignLeft);
        this->connect(
            bitsetWidget,
            &BitsetWidget::byteChanged,
            this,
            &TargetRegisterInspectorWindow::updateRegisterValueInputField
        );
        this->bitsetWidgets.push_back(bitsetWidget);

        if (((registerByteIndex + 1) % 2) == 0) {
            registerBitsetWidgetLayout->addLayout(bitsetSingleHorizontalLayout);
            bitsetSingleHorizontalLayout = new QHBoxLayout();
            bitsetSingleHorizontalLayout->setSpacing(BitWidget::SPACING);
            bitsetSingleHorizontalLayout->setContentsMargins(0, 0, 0, 0);
        }

        byteNumber--;
    }

    registerBitsetWidgetLayout->addLayout(bitsetSingleHorizontalLayout);
    registerBitsetWidgetLayout->addStretch(1);

    this->registerHistoryWidget->setFixedHeight(this->contentContainer->sizeHint().height());

    this->connect(this->helpButton, &QPushButton::clicked, this, &TargetRegisterInspectorWindow::openHelpPage);
    this->connect(this->closeButton, &QPushButton::clicked, this, &QWidget::close);
    this->connect(
        this->refreshValueButton,
        &QPushButton::clicked,
        this,
        &TargetRegisterInspectorWindow::refreshRegisterValue
    );
    this->connect(this->applyButton, &QPushButton::clicked, this, &TargetRegisterInspectorWindow::applyChanges);

    this->connect(
        this->registerHistoryWidget,
        &RegisterHistoryWidget::historyItemSelected,
        this,
        &TargetRegisterInspectorWindow::onHistoryItemSelected
    );

    this->connect(
        this->registerValueTextInput,
        &QLineEdit::textEdited,
        this,
        &TargetRegisterInspectorWindow::onValueTextInputChanged
    );

    this->connect(
        &insightWorker,
        &InsightWorker::targetStateUpdated,
        this,
        &TargetRegisterInspectorWindow::onTargetStateChanged
    );

    this->updateRegisterValueInputField();
    this->onTargetStateChanged(currentTargetState);
    this->show();
}

bool TargetRegisterInspectorWindow::registerSupported(const Targets::TargetRegisterDescriptor& descriptor) {
    return (descriptor.size > 0 && descriptor.size <= 8);
}

void TargetRegisterInspectorWindow::setValue(const Targets::TargetMemoryBuffer& registerValue) {
    this->registerValue = registerValue;
    this->registerHistoryWidget->updateCurrentItemValue(this->registerValue);
    this->registerHistoryWidget->selectCurrentItem();
}

void TargetRegisterInspectorWindow::onValueTextInputChanged(QString text) {
    if (text.isEmpty()) {
        text = "0";
    }

    bool validHexValue = false;
    text.toLongLong(&validHexValue, 16);
    if (!validHexValue) {
        return;
    }

    auto registerSize = this->registerDescriptor.size;
    auto newValue = QByteArray::fromHex(
        text.remove("0x", Qt::CaseInsensitive).toLatin1()
    ).rightJustified(registerSize, 0).right(registerSize);

    assert(newValue.size() >= registerSize);
    assert(registerValue.size() == registerSize);
    for (std::uint32_t byteIndex = 0; byteIndex < registerSize; byteIndex++) {
        this->registerValue.at(byteIndex) = static_cast<unsigned char>(newValue.at(byteIndex));
    }

    for (auto& bitsetWidget : this->bitsetWidgets) {
        bitsetWidget->updateValue();
    }
}

void TargetRegisterInspectorWindow::onTargetStateChanged(TargetState newState) {
    if (newState != TargetState::STOPPED) {
        this->registerValueTextInput->setDisabled(true);
        this->registerValueBitsetWidgetContainer->setDisabled(true);
        this->applyButton->setDisabled(true);
        this->refreshValueButton->setDisabled(true);

    } else if (this->targetState != TargetState::STOPPED && this->registerValueContainer->isEnabled()) {
        this->registerValueTextInput->setDisabled(false);
        this->registerValueBitsetWidgetContainer->setDisabled(false);
        this->applyButton->setDisabled(false);
        this->refreshValueButton->setDisabled(false);
    }

    this->targetState = newState;
}

void TargetRegisterInspectorWindow::onHistoryItemSelected(const Targets::TargetMemoryBuffer& selectedRegisterValue) {
    this->registerValue = selectedRegisterValue;
    this->updateValue();

    if (this->registerHistoryWidget->isCurrentItemSelected()) {
        this->refreshValueButton->setVisible(true);

    } else {
        this->refreshValueButton->setVisible(false);
    }
}

void TargetRegisterInspectorWindow::updateRegisterValueInputField() {
    auto value = QByteArray(
        reinterpret_cast<const char*>(this->registerValue.data()),
        static_cast<qsizetype>(this->registerValue.size())
    );

    this->registerValueTextInput->setText("0x" + QString(value.toHex()).toUpper());
}

void TargetRegisterInspectorWindow::updateRegisterValueBitsetWidgets() {
    for (auto& bitsetWidget : this->bitsetWidgets) {
        bitsetWidget->updateValue();
    }
}

void TargetRegisterInspectorWindow::updateValue() {
    this->updateRegisterValueInputField();
    this->updateRegisterValueBitsetWidgets();
}

void TargetRegisterInspectorWindow::refreshRegisterValue() {
    this->registerValueContainer->setDisabled(true);
    auto readTargetRegisterTask = new ReadTargetRegisters({this->registerDescriptor});

    this->connect(
        readTargetRegisterTask,
        &ReadTargetRegisters::targetRegistersRead,
        this,
        [this] (Targets::TargetRegisters targetRegisters) {
            this->registerValueContainer->setDisabled(false);

            for (const auto& targetRegister : targetRegisters) {
                if (targetRegister.descriptor == this->registerDescriptor) {
                    this->setValue(targetRegister.value);
                }
            }
        }
    );

    this->connect(
        readTargetRegisterTask,
        &InsightWorkerTask::failed,
        this,
        [this] {
            this->registerValueContainer->setDisabled(false);
        }
    );

    this->insightWorker.queueTask(readTargetRegisterTask);
}

void TargetRegisterInspectorWindow::applyChanges() {
    this->registerValueContainer->setDisabled(true);
    const auto targetRegister = Targets::TargetRegister(this->registerDescriptor, this->registerValue);
    auto writeRegisterTask = new WriteTargetRegister(targetRegister);

    this->connect(writeRegisterTask, &InsightWorkerTask::completed, this, [this, targetRegister] {
        this->registerValueContainer->setDisabled(false);
        emit this->insightWorker.targetRegistersWritten(
            {targetRegister},
            DateTime::currentDateTime()
        );
        this->registerHistoryWidget->updateCurrentItemValue(targetRegister.value);
        this->registerHistoryWidget->selectCurrentItem();
    });

    this->connect(writeRegisterTask, &InsightWorkerTask::failed, this, [this] {
        // TODO: Let the user know the write failed.
        this->registerValueContainer->setDisabled(false);
    });

    this->insightWorker.queueTask(writeRegisterTask);
}

void TargetRegisterInspectorWindow::openHelpPage() {
//    QDesktopServices::openUrl(QUrl("https://bloom.oscillate.io/docs/register-inspection"));
    QDesktopServices::openUrl(QUrl("http://bloom.local/docs/register-inspection"));
}
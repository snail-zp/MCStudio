#include "presentation/workstationcalibrationpage.h"
#include <QAbstractItemView>
#include "presentation/uihelpers.h"

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSaveFile>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QShowEvent>
#include <QHideEvent>

#include <algorithm>

namespace
{
enum EditorColumn
{
    WorkstationNameColumn = 0,
    WorkstationDescriptionColumn,
    ModuleKeyColumn,
    ModuleNameColumn,
    ModuleDescriptionColumn,
    AxisNameColumn,
    AxisNumberColumn,
    AxisSpeedColumn,
    AxisStepColumn,
    AxisUnitColumn,
    EditorColumnCount
};

enum WorkstationEditorColumn
{
    WorkstationEditorNameColumn = 0,
    WorkstationEditorDescriptionColumn,
    WorkstationEditorStartCommandColumn,
    WorkstationEditorStartDoneVariableColumn,
    WorkstationEditorStartDoneBufferColumn,
    WorkstationEditorStartDoneValueColumn,
    WorkstationEditorUnifiedStepColumn,
    WorkstationEditorJogLowColumn,
    WorkstationEditorJogHighColumn,
    WorkstationEditorColumnCount
};

enum MoveEditorColumn
{
    MoveEditorWorkstationColumn = 0,
    MoveEditorStepColumn,
    MoveEditorAxisColumn,
    MoveEditorTargetColumn,
    MoveEditorSensorColumn,
    MoveEditorColumnCount
};

enum OperationEditorColumn
{
    OperationEditorWorkstationColumn = 0,
    OperationEditorModuleKeyColumn,
    OperationEditorModuleNameColumn,
    OperationEditorOperationKeyColumn,
    OperationEditorCommandColumn,
    OperationEditorDoneVariableColumn,
    OperationEditorDoneBufferColumn,
    OperationEditorDoneValueColumn,
    OperationEditorColumnCount
};

QString trText(const QString& languageCode, const QString& english, const QString& chinese)
{
    return languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0 ? english : chinese;
}

QString axisTypeOf(const CalibrationAxis& axis)
{
    const QString name = axis.name.trimmed().toUpper();
    if (name.startsWith(QStringLiteral("X"))) return QStringLiteral("X");
    if (name.startsWith(QStringLiteral("Y"))) return QStringLiteral("Y");
    if (name.startsWith(QStringLiteral("Z"))) return QStringLiteral("Z");
    if (name.startsWith(QStringLiteral("T"))) return QStringLiteral("T");
    return QString();
}

bool isRobotArmModule(const CalibrationModule& module)
{
    return module.key.compare(QStringLiteral("robotArm"), Qt::CaseInsensitive) == 0
        || module.key.compare(QStringLiteral("fork"), Qt::CaseInsensitive) == 0
        || module.name.contains(QStringLiteral("robot"), Qt::CaseInsensitive)
        || module.name.contains(QStringLiteral("fork"), Qt::CaseInsensitive);
}

bool isForkModule(const CalibrationModule& module)
{
    return module.key.compare(QStringLiteral("fork"), Qt::CaseInsensitive) == 0
        || module.name.contains(QStringLiteral("fork"), Qt::CaseInsensitive);
}

bool isChuckLpModule(const CalibrationModule& module)
{
    return module.key.compare(QStringLiteral("chuckLP"), Qt::CaseInsensitive) == 0
        || module.name.contains(QStringLiteral("chuck"), Qt::CaseInsensitive);
}

bool isEdgeChuckModule(const CalibrationModule& module)
{
    return module.key.compare(QStringLiteral("edgeChuck"), Qt::CaseInsensitive) == 0
        || module.key.compare(QStringLiteral("edge_chuck"), Qt::CaseInsensitive) == 0
        || module.name.contains(QStringLiteral("edge"), Qt::CaseInsensitive);
}

bool isMainChuckModule(const CalibrationModule& module)
{
    return module.key.compare(QStringLiteral("mainChuck"), Qt::CaseInsensitive) == 0
        || module.key.compare(QStringLiteral("main_chuck"), Qt::CaseInsensitive) == 0
        || module.key.compare(QStringLiteral("chuckLP"), Qt::CaseInsensitive) == 0
        || module.name.contains(QStringLiteral("main"), Qt::CaseInsensitive);
}

bool isAlignerModule(const CalibrationModule& module)
{
    return module.key.compare(QStringLiteral("aligner"), Qt::CaseInsensitive) == 0
        || module.name.contains(QStringLiteral("aligner"), Qt::CaseInsensitive);
}

QFrame* createCard(QWidget* parent, int minWidth = 0)
{
    auto* card = new QFrame(parent);
    card->setStyleSheet(QStringLiteral(
        "QFrame{background:transparent;border:none;}"));
    if (minWidth > 0) {
        card->setMinimumWidth(minWidth);
    }
    return card;
}

QPushButton* createRoundJogButton(const QString& text, QWidget* parent)
{
    auto* button = new QPushButton(text, parent);
    button->setFixedSize(46, 46);
    button->setStyleSheet(QStringLiteral(
        "QPushButton{background:#ffffff;border:1px solid #bfd1e2;border-radius:23px;"
        "font-size:18px;font-weight:700;color:#214c74;}"
        "QPushButton:hover{background:#eef5fb;border-color:#8eb0d0;}"
        "QPushButton:pressed{background:#dbe9f5;}"));
    return button;
}

QLabel* createSectionTitle(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet(QStringLiteral("font-size:13px;font-weight:700;color:#18324a;"));
    return label;
}

QLabel* createMiniLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet(QStringLiteral("font-size:11px;color:#5a6d7f;"));
    return label;
}

QLineEdit* createPositionDisplay(const CalibrationAxis& axis, QWidget* parent)
{
    auto* edit = new QLineEdit(QStringLiteral("0.000"), parent);
    edit->setReadOnly(true);
    edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    edit->setFixedWidth(64);
    edit->setToolTip(QStringLiteral("%1 (#%2)").arg(axis.name).arg(axis.axisNumber));
    edit->setStyleSheet(QStringLiteral(
        "QLineEdit{background:transparent;border:none;"
        "padding:0 2px;font-size:11px;font-weight:600;color:#1f3447;}"));
    return edit;
}

QTableWidgetItem* makeItem(const QString& text)
{
    return new QTableWidgetItem(text);
}

bool containsString(const QStringList& values, const QString& value)
{
    return values.contains(value);
}

void addUniqueString(QStringList& values, const QString& value)
{
    if (!values.contains(value)) {
        values.append(value);
    }
}

void removeString(QStringList& values, const QString& value)
{
    values.removeAll(value);
}

void clearLayoutWidgets(QLayout* layout)
{
    if (!layout) {
        return;
    }

    while (layout->count() > 0) {
        QLayoutItem* item = layout->takeAt(0);
        if (QWidget* widget = item->widget()) {
            widget->hide();
            widget->setParent(nullptr);
            widget->deleteLater();
        } else if (QLayout* childLayout = item->layout()) {
            clearLayoutWidgets(childLayout);
            delete childLayout;
        }
        delete item;
    }
}

QString arrowUp()
{
    return QString(QChar(0x2191));
}

QString arrowDown()
{
    return QString(QChar(0x2193));
}

QString arrowLeft()
{
    return QString(QChar(0x2190));
}

QString arrowRight()
{
    return QString(QChar(0x2192));
}

QString rotateCw()
{
    return QString(QChar(0x21BB));
}

QString rotateCcw()
{
    return QString(QChar(0x21BA));
}

QString normalizeOperationKey(const QString& rawKey)
{
    const QString key = rawKey.trimmed().toLower();
    if (key == QStringLiteral("realse")) {
        return QStringLiteral("release");
    }
    return key;
}

QStringList supportedOperationKeys()
{
    return QStringList()
        << QStringLiteral("grip")
        << QStringLiteral("release");
}

bool moduleHasOperation(const CalibrationModule& module, const QString& operationKey)
{
    const QString normalizedTarget = normalizeOperationKey(operationKey);
    for (auto it = module.operations.cbegin(); it != module.operations.cend(); ++it) {
        if (normalizeOperationKey(it.key()) == normalizedTarget) {
            return true;
        }
    }
    return false;
}

CalibrationCommandAction moduleOperationAction(const CalibrationModule& module, const QString& operationKey)
{
    const QString normalizedTarget = normalizeOperationKey(operationKey);
    for (auto it = module.operations.cbegin(); it != module.operations.cend(); ++it) {
        if (normalizeOperationKey(it.key()) == normalizedTarget) {
            return it.value();
        }
    }
    return {};
}

QComboBox* createOperationKeyCombo(QWidget* parent, const QString& currentKey)
{
    auto* combo = new QComboBox(parent);
    combo->addItems(supportedOperationKeys());
    combo->setEditable(false);
    combo->setMinimumWidth(120);
    const QString normalizedKey = normalizeOperationKey(currentKey);
    const int index = combo->findText(normalizedKey);
    combo->setCurrentIndex(index >= 0 ? index : 0);
    return combo;
}

QIcon createClampActionIcon(bool grip)
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QColor accent = grip ? QColor(QStringLiteral("#1f6f43")) : QColor(QStringLiteral("#2b5f8a"));
    const QColor softFill = grip ? QColor(QStringLiteral("#dff3e8")) : QColor(QStringLiteral("#e4eef9"));

    painter.setPen(Qt::NoPen);
    painter.setBrush(softFill);
    painter.drawRoundedRect(QRectF(4.0, 7.0, 24.0, 18.0), 8.0, 8.0);

    painter.setPen(QPen(accent, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(QPointF(8.5, 11.0), QPointF(8.5, 21.0));
    painter.drawLine(QPointF(23.5, 11.0), QPointF(23.5, 21.0));
    painter.drawLine(QPointF(8.5, 11.0), QPointF(12.5, 7.5));
    painter.drawLine(QPointF(23.5, 11.0), QPointF(19.5, 7.5));
    painter.drawLine(QPointF(8.5, 21.0), QPointF(12.5, 24.5));
    painter.drawLine(QPointF(23.5, 21.0), QPointF(19.5, 24.5));

    painter.setBrush(accent);
    if (grip) {
        QPolygonF leftArrow;
        leftArrow << QPointF(13.5, 16.0) << QPointF(17.0, 13.2) << QPointF(17.0, 18.8);
        QPolygonF rightArrow;
        rightArrow << QPointF(18.5, 16.0) << QPointF(15.0, 13.2) << QPointF(15.0, 18.8);
        painter.drawPolygon(leftArrow);
        painter.drawPolygon(rightArrow);
    } else {
        QPolygonF leftArrow;
        leftArrow << QPointF(10.5, 16.0) << QPointF(14.0, 13.2) << QPointF(14.0, 18.8);
        QPolygonF rightArrow;
        rightArrow << QPointF(21.5, 16.0) << QPointF(18.0, 13.2) << QPointF(18.0, 18.8);
        painter.drawPolygon(leftArrow);
        painter.drawPolygon(rightArrow);
    }

    return QIcon(pixmap);
}
}

WorkstationCalibrationPage::WorkstationCalibrationPage(ControllerService& controllerService, QWidget* parent)
    : LoggedPageWidget(FileLogger::Category::WorkstationCalibration, parent)
    , m_controllerService(controllerService)
    , m_executionService(controllerService)
    , m_workflowService(controllerService, m_service)
    , m_configFilePath(QDir::current().filePath(QStringLiteral("Config/workstation_calibration_simulator.json")))
{
    buildUi();
    updateWatcher();
    retranslateUi();
}

void WorkstationCalibrationPage::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode.trimmed().compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("en-US")
        : QStringLiteral("zh-CN");
    retranslateUi();
    if (m_initialConfigLoaded) {
        rebuildCalibrationView();
    }
    if (m_configEditorDialog) {
        populateEditorTable();
    }
}

void WorkstationCalibrationPage::reloadConfig()
{
    // Disable watchers and timers during reload to prevent dangling pointer access
    if (m_configWatcher) {
        m_configWatcher->blockSignals(true);
    }
    if (m_positionRefreshTimer) {
        m_positionRefreshTimer->stop();
    }

    QString errorMessage;
    const QVector<WorkstationCalibration> calibrations = m_service.loadCalibrations(configFilePath(), &errorMessage);

    if (calibrations.isEmpty()) {
        m_calibrations.clear();
        m_selectedWorkstationName.clear();
        if (m_workstationCombo) {
            QSignalBlocker blocker(m_workstationCombo);
            m_workstationCombo->clear();
        }
        rebuildCalibrationView();
        if (m_configEditorDialog) {
            populateEditorTable();
        }
        showStatusMessage(errorMessage.isEmpty() ? textForKey(QStringLiteral("empty")) : errorMessage, true);
        
        // Re-enable watchers and timers
        if (m_configWatcher) {
            m_configWatcher->blockSignals(false);
        }
        updatePositionRefreshState();
        return;
    }

    const QString previousSelection = !m_selectedWorkstationName.isEmpty()
        ? m_selectedWorkstationName
        : ((m_workstationCombo && m_workstationCombo->currentIndex() >= 0)
               ? m_workstationCombo->currentText()
               : QString());

    m_calibrations = calibrations;
    m_lowJogSpeedWorkstations.clear();
    for (const WorkstationCalibration& calibration : m_calibrations) {
        addUniqueString(m_lowJogSpeedWorkstations, calibration.calibrationName);
    }

    if (m_workstationCombo) {
        QSignalBlocker blocker(m_workstationCombo);
        m_workstationCombo->clear();
        for (const WorkstationCalibration& calibration : m_calibrations) {
            m_workstationCombo->addItem(calibration.calibrationName);
        }

        int index = 0;
        if (!previousSelection.isEmpty()) {
            const int previousIndex = m_workstationCombo->findText(previousSelection);
            if (previousIndex >= 0) {
                index = previousIndex;
            }
        }
        m_workstationCombo->setCurrentIndex(index);
        m_selectedWorkstationName = m_workstationCombo->currentText();
    }

    rebuildCalibrationView();
    if (m_configEditorDialog) {
        populateEditorTable();
    }
    updateWatcher();
    refreshHeaderControls();
    showStatusMessage(textForKey(QStringLiteral("loaded")).arg(m_calibrations.size()), false);
    
    // Re-enable watchers and timers after reload is complete
    if (m_configWatcher) {
        m_configWatcher->blockSignals(false);
    }
    updatePositionRefreshState();
}

void WorkstationCalibrationPage::handleWorkstationChanged(int)
{
    if (QComboBox* combo = qobject_cast<QComboBox*>(sender())) {
        m_selectedWorkstationName = combo->currentText();
        if (combo != m_workstationCombo && m_workstationCombo) {
            const QSignalBlocker blocker(m_workstationCombo);
            const int index = m_workstationCombo->findText(m_selectedWorkstationName);
            if (index >= 0) {
                m_workstationCombo->setCurrentIndex(index);
            }
        }
    } else if (m_workstationCombo) {
        m_selectedWorkstationName = m_workstationCombo->currentText();
    }

    // Rebuild on the next event-loop turn so the emitting combo can finish
    // processing its index change before the current content tree is deleted.
    QTimer::singleShot(0, this, [this]() {
        refreshHeaderControls();
        rebuildCalibrationView();
    });
}

void WorkstationCalibrationPage::openConfigEditorDialog()
{
    if (!m_configEditorDialog) {
        buildConfigEditorDialog();
        retranslateUi();
    }

    populateEditorTable();
    m_configEditorDialog->show();
    m_configEditorDialog->raise();
    m_configEditorDialog->activateWindow();
}

void WorkstationCalibrationPage::browseConfigFile()
{
    const QString selectedPath = QFileDialog::getOpenFileName(
        this,
        trText(m_languageCode,
               QStringLiteral("Select Calibration Config"),
               QStringLiteral("\u9009\u62e9\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e")),
        QFileInfo(configFilePath()).absolutePath(),
        QStringLiteral("JSON Files (*.json)"));
    if (selectedPath.isEmpty()) {
        return;
    }

    m_configFilePath = selectedPath;
    updateWatcher();
    retranslateUi();
    reloadConfig();
    showStatusMessage(textForKey(QStringLiteral("switch_success")).arg(selectedPath), false);
}

void WorkstationCalibrationPage::addAxisRow()
{
    if (!m_editorTabWidget) {
        return;
    }

    const WorkstationCalibration calibration = m_calibrations.isEmpty() ? WorkstationCalibration() : m_calibrations.first();
    const CalibrationModule module = calibration.modules.isEmpty() ? CalibrationModule() : calibration.modules.first();
    const int currentTab = m_editorTabWidget->currentIndex();

    if (currentTab == 0 && m_editorWorkstationTable) {
        const int row = m_editorWorkstationTable->rowCount();
        m_editorWorkstationTable->insertRow(row);
        m_editorWorkstationTable->setItem(row, WorkstationEditorNameColumn, makeItem(calibration.calibrationName.isEmpty() ? QStringLiteral("Station") : calibration.calibrationName));
        m_editorWorkstationTable->setItem(row, WorkstationEditorDescriptionColumn, makeItem(calibration.description));
        m_editorWorkstationTable->setItem(row, WorkstationEditorStartCommandColumn, makeItem(QStringLiteral("?FPOS(0)")));
        m_editorWorkstationTable->setItem(row, WorkstationEditorStartDoneVariableColumn, makeItem(QStringLiteral("start_done")));
        m_editorWorkstationTable->setItem(row, WorkstationEditorStartDoneBufferColumn, makeItem(QStringLiteral("-1")));
        m_editorWorkstationTable->setItem(row, WorkstationEditorStartDoneValueColumn, makeItem(QStringLiteral("1")));
        m_editorWorkstationTable->setItem(row, WorkstationEditorUnifiedStepColumn, makeItem(QStringLiteral("1.000")));
        m_editorWorkstationTable->setItem(row, WorkstationEditorJogLowColumn, makeItem(QStringLiteral("10.000")));
        m_editorWorkstationTable->setItem(row, WorkstationEditorJogHighColumn, makeItem(QStringLiteral("30.000")));
        m_editorWorkstationTable->setCurrentCell(row, WorkstationEditorNameColumn);
        return;
    }

    if (currentTab == 1 && m_editorTable) {
        const int row = m_editorTable->rowCount();
        m_editorTable->insertRow(row);
        m_editorTable->setItem(row, WorkstationNameColumn, makeItem(calibration.calibrationName.isEmpty() ? QStringLiteral("Station") : calibration.calibrationName));
        m_editorTable->setItem(row, WorkstationDescriptionColumn, makeItem(calibration.description));
        m_editorTable->setItem(row, ModuleKeyColumn, makeItem(module.key.isEmpty() ? QStringLiteral("module") : module.key));
        m_editorTable->setItem(row, ModuleNameColumn, makeItem(module.name.isEmpty() ? QStringLiteral("Module") : module.name));
        m_editorTable->setItem(row, ModuleDescriptionColumn, makeItem(module.description));
        m_editorTable->setItem(row, AxisNameColumn, makeItem(QStringLiteral("Axis")));
        m_editorTable->setItem(row, AxisNumberColumn, makeItem(QStringLiteral("1")));
        m_editorTable->setItem(row, AxisSpeedColumn, makeItem(QStringLiteral("10.000")));
        m_editorTable->setItem(row, AxisStepColumn, makeItem(QStringLiteral("1.000")));
        m_editorTable->setItem(row, AxisUnitColumn, makeItem(QStringLiteral("mm")));
        m_editorTable->setCurrentCell(row, AxisNameColumn);
        return;
    }

    if (currentTab == 2 && m_editorOperationTable) {
        const int row = m_editorOperationTable->rowCount();
        m_editorOperationTable->insertRow(row);
        m_editorOperationTable->setItem(row, OperationEditorWorkstationColumn, makeItem(calibration.calibrationName.isEmpty() ? QStringLiteral("Station") : calibration.calibrationName));
        m_editorOperationTable->setItem(row, OperationEditorModuleKeyColumn, makeItem(module.key.isEmpty() ? QStringLiteral("robotArm") : module.key));
        m_editorOperationTable->setItem(row, OperationEditorModuleNameColumn, makeItem(module.name.isEmpty() ? QStringLiteral("Robot Arm") : module.name));
        m_editorOperationTable->setCellWidget(row, OperationEditorOperationKeyColumn, createOperationKeyCombo(m_editorOperationTable, QStringLiteral("grip")));
        m_editorOperationTable->setItem(row, OperationEditorCommandColumn, makeItem(QStringLiteral("GRIP_COMMAND;")));
        m_editorOperationTable->setItem(row, OperationEditorDoneVariableColumn, makeItem(QStringLiteral("grip_done")));
        m_editorOperationTable->setItem(row, OperationEditorDoneBufferColumn, makeItem(QStringLiteral("-1")));
        m_editorOperationTable->setItem(row, OperationEditorDoneValueColumn, makeItem(QStringLiteral("1")));
        m_editorOperationTable->setCurrentCell(row, OperationEditorOperationKeyColumn);
        return;
    }

    if (currentTab == 3 && m_editorMoveTable) {
        const int row = m_editorMoveTable->rowCount();
        m_editorMoveTable->insertRow(row);
        m_editorMoveTable->setItem(row, MoveEditorWorkstationColumn, makeItem(calibration.calibrationName.isEmpty() ? QStringLiteral("Station") : calibration.calibrationName));
        m_editorMoveTable->setItem(row, MoveEditorStepColumn, makeItem(QString::number(row + 1)));
        m_editorMoveTable->setItem(row, MoveEditorAxisColumn, makeItem(QStringLiteral("X1")));
        m_editorMoveTable->setItem(row, MoveEditorTargetColumn, makeItem(QStringLiteral("0.000")));
        m_editorMoveTable->setItem(row, MoveEditorSensorColumn, makeItem(QString()));
        m_editorMoveTable->setCurrentCell(row, MoveEditorAxisColumn);
    }
}

void WorkstationCalibrationPage::removeSelectedAxisRow()
{
    if (!m_editorTabWidget) {
        return;
    }

    const int currentTab = m_editorTabWidget->currentIndex();
    if (currentTab == 0 && m_editorWorkstationTable) {
        const int row = m_editorWorkstationTable->currentRow();
        if (row >= 0) {
            m_editorWorkstationTable->removeRow(row);
        }
        return;
    }
    if (currentTab == 1 && m_editorTable) {
        const int row = m_editorTable->currentRow();
        if (row >= 0) {
            m_editorTable->removeRow(row);
        }
        return;
    }
    if (currentTab == 2 && m_editorOperationTable) {
        const int row = m_editorOperationTable->currentRow();
        if (row >= 0) {
            m_editorOperationTable->removeRow(row);
        }
        return;
    }
    if (currentTab == 3 && m_editorMoveTable) {
        const int row = m_editorMoveTable->currentRow();
        if (row >= 0) {
            m_editorMoveTable->removeRow(row);
        }
    }
}

void WorkstationCalibrationPage::saveCalibrationConfig()
{
    QVector<WorkstationCalibration> calibrations;
    QString errorMessage;
    if (!collectCalibrationsFromEditor(&calibrations, &errorMessage)) {
        QMessageBox::warning(
            this,
            trText(m_languageCode, QStringLiteral("Save Failed"), QStringLiteral("\u4fdd\u5b58\u5931\u8d25")),
            errorMessage);
        showStatusMessage(errorMessage, true);
        return;
    }

    if (!m_service.saveCalibrations(configFilePath(), calibrations, &errorMessage)) {
        QMessageBox::warning(
            this,
            trText(m_languageCode, QStringLiteral("Save Failed"), QStringLiteral("\u4fdd\u5b58\u5931\u8d25")),
            errorMessage);
        showStatusMessage(errorMessage, true);
        return;
    }

    m_calibrations = calibrations;
    updateWatcher();
    reloadConfig();
    showStatusMessage(textForKey(QStringLiteral("save_success")), false);
}

void WorkstationCalibrationPage::handleConfigFileChanged(const QString& path)
{
    Q_UNUSED(path);
    QTimer::singleShot(120, this, [this]() {
        updateWatcher();
        reloadConfig();
        showStatusMessage(textForKey(QStringLiteral("reload_success")), false);
    });
}

void WorkstationCalibrationPage::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(36, 28, 36, 28);
    rootLayout->setSpacing(14);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_workstationLabel = new QLabel(this);
    m_workstationCombo = new QComboBox(this);
    m_workstationCombo->setMinimumWidth(180);
    m_headerSettingsButton = createSettingsLaunchButton(this);

    auto* topRowLayout = new QHBoxLayout();
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(10);
    topRowLayout->addWidget(m_statusLabel, 1);
    topRowLayout->addWidget(m_headerSettingsButton, 0, Qt::AlignTop);

    m_workstationSwitchContainer = new QWidget(this);
    auto* workstationRowLayout = new QHBoxLayout(m_workstationSwitchContainer);
    workstationRowLayout->setContentsMargins(0, 0, 0, 0);
    workstationRowLayout->setSpacing(8);
    workstationRowLayout->addWidget(m_workstationLabel);
    workstationRowLayout->addWidget(m_workstationCombo, 0, Qt::AlignLeft);
    workstationRowLayout->addStretch();
    m_workstationSwitchContainer->hide();

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_positionStatusBar = new QWidget(this);
    m_positionStatusBar->setStyleSheet(QStringLiteral(
        "QWidget{background:#ffffff;border-top:1px solid #d7dee5;}"));
    m_positionStatusLayout = new QHBoxLayout(m_positionStatusBar);
    m_positionStatusLayout->setContentsMargins(12, 6, 12, 6);
    m_positionStatusLayout->setSpacing(12);

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(12);
    m_scrollArea->setWidget(m_contentWidget);

    m_runtimeLogLabel = new QLabel(this);
    m_runtimeLogLabel->hide();
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(180);
    m_logView->setStyleSheet(UiHelpers::logViewStyle());
    bindFeedbackWidgets(m_statusLabel, m_logView);

    rootLayout->addLayout(topRowLayout);
    rootLayout->addWidget(m_scrollArea, 1);
    rootLayout->addWidget(m_logView);

    m_configWatcher = new QFileSystemWatcher(this);
    m_positionRefreshTimer = new QTimer(this);
    m_positionRefreshTimer->setInterval(350);
    m_positionStatusBar->hide();

    connect(m_workstationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WorkstationCalibrationPage::handleWorkstationChanged);
    connect(m_configWatcher, &QFileSystemWatcher::fileChanged, this, &WorkstationCalibrationPage::handleConfigFileChanged);
    connect(m_positionRefreshTimer, &QTimer::timeout, this, &WorkstationCalibrationPage::refreshAxisPositions);
}

void WorkstationCalibrationPage::buildConfigEditorDialog()
{
    if (m_configEditorDialog) {
        return;
    }

    m_configEditorDialog = new QDialog(this);
    m_configEditorDialog->setModal(false);

    auto* rootLayout = new QVBoxLayout(m_configEditorDialog);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* toolbarFrame = new QFrame(m_configEditorDialog);
    toolbarFrame->setStyleSheet(QStringLiteral(
        "QFrame{background:#f7f9fc;border-bottom:1px solid #d7dee5;}"));
    auto* toolbarLayout = new QVBoxLayout(toolbarFrame);
    toolbarLayout->setContentsMargins(8, 0, 8, 6);
    toolbarLayout->setSpacing(4);

    auto* fileRow = new QHBoxLayout();
    fileRow->setContentsMargins(0, 0, 0, 0);
    fileRow->setSpacing(6);
    m_editorConfigFileLabel = new QLabel(toolbarFrame);
    m_editorConfigFileLabel->setMinimumWidth(72);
    m_editorConfigFileLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_editorConfigFileEdit = new QLineEdit(toolbarFrame);
    m_editorConfigFileEdit->setReadOnly(true);
    m_editorReloadButton = new QPushButton(toolbarFrame);
    m_editorBrowseButton = new QPushButton(toolbarFrame);
    fileRow->addWidget(m_editorConfigFileLabel);
    fileRow->addWidget(m_editorConfigFileEdit, 1);
    fileRow->addWidget(m_editorReloadButton);
    fileRow->addWidget(m_editorBrowseButton);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setContentsMargins(0, 0, 0, 0);
    buttonRow->setSpacing(6);
    m_editorAddButton = new QPushButton(toolbarFrame);
    m_editorRemoveButton = new QPushButton(toolbarFrame);
    m_editorSaveButton = new QPushButton(toolbarFrame);
    buttonRow->addWidget(m_editorAddButton);
    buttonRow->addWidget(m_editorRemoveButton);
    buttonRow->addStretch();
    buttonRow->addWidget(m_editorSaveButton);
    toolbarLayout->addLayout(fileRow);
    toolbarLayout->addLayout(buttonRow);

    m_editorWorkstationSectionLabel = new QLabel(m_configEditorDialog);
    m_editorWorkstationSectionLabel->setStyleSheet(QStringLiteral("font-size:13px;font-weight:700;color:#18324a;"));
    m_editorWorkstationTable = new QTableWidget(m_configEditorDialog);
    m_editorWorkstationTable->setColumnCount(WorkstationEditorColumnCount);
    m_editorWorkstationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_editorWorkstationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorWorkstationTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                              | QAbstractItemView::SelectedClicked
                                              | QAbstractItemView::EditKeyPressed);
    m_editorWorkstationTable->verticalHeader()->setVisible(false);
    m_editorWorkstationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_editorWorkstationTable->setAlternatingRowColors(true);
    m_editorWorkstationTable->setMinimumHeight(220);

    m_editorMoveSectionLabel = new QLabel(m_configEditorDialog);
    m_editorMoveSectionLabel->setStyleSheet(QStringLiteral("font-size:13px;font-weight:700;color:#18324a;"));

    m_editorMoveTable = new QTableWidget(m_configEditorDialog);
    m_editorMoveTable->setColumnCount(MoveEditorColumnCount);
    m_editorMoveTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_editorMoveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorMoveTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                       | QAbstractItemView::SelectedClicked
                                       | QAbstractItemView::EditKeyPressed);
    m_editorMoveTable->verticalHeader()->setVisible(false);
    m_editorMoveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_editorMoveTable->setAlternatingRowColors(true);
    m_editorMoveTable->setMinimumHeight(220);

    m_editorOperationSectionLabel = new QLabel(m_configEditorDialog);
    m_editorOperationSectionLabel->setStyleSheet(QStringLiteral("font-size:13px;font-weight:700;color:#18324a;"));

    m_editorOperationTable = new QTableWidget(m_configEditorDialog);
    m_editorOperationTable->setColumnCount(OperationEditorColumnCount);
    m_editorOperationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_editorOperationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorOperationTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                            | QAbstractItemView::SelectedClicked
                                            | QAbstractItemView::EditKeyPressed);
    m_editorOperationTable->verticalHeader()->setVisible(false);
    m_editorOperationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_editorOperationTable->setAlternatingRowColors(true);
    m_editorOperationTable->setMinimumHeight(180);

    m_editorTable = new QTableWidget(m_configEditorDialog);
    m_editorTable->setColumnCount(EditorColumnCount);
    m_editorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_editorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorTable->setEditTriggers(QAbstractItemView::DoubleClicked
                                   | QAbstractItemView::SelectedClicked
                                   | QAbstractItemView::EditKeyPressed);
    m_editorTable->verticalHeader()->setVisible(false);
    m_editorTable->horizontalHeader()->setStretchLastSection(false);
    m_editorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_editorTable->setAlternatingRowColors(true);

    m_editorTabWidget = new QTabWidget(m_configEditorDialog);
    m_editorTabWidget->addTab(m_editorWorkstationTable, QString());
    m_editorTabWidget->addTab(m_editorTable, QString());
    m_editorTabWidget->addTab(m_editorOperationTable, QString());
    m_editorTabWidget->addTab(m_editorMoveTable, QString());

    rootLayout->addWidget(toolbarFrame);
    rootLayout->addWidget(m_editorTabWidget, 1);

    connect(m_editorReloadButton, &QPushButton::clicked, this, &WorkstationCalibrationPage::reloadConfig);
    connect(m_editorBrowseButton, &QPushButton::clicked, this, &WorkstationCalibrationPage::browseConfigFile);
    connect(m_editorAddButton, &QPushButton::clicked, this, &WorkstationCalibrationPage::addAxisRow);
    connect(m_editorRemoveButton, &QPushButton::clicked, this, &WorkstationCalibrationPage::removeSelectedAxisRow);
    connect(m_editorSaveButton, &QPushButton::clicked, this, &WorkstationCalibrationPage::saveCalibrationConfig);
}

void WorkstationCalibrationPage::retranslateUi()
{
    if (m_workstationLabel) m_workstationLabel->setText(textForKey(QStringLiteral("workstation")));
    if (m_runtimeLogLabel) m_runtimeLogLabel->setText(textForKey(QStringLiteral("runtime_log")));
    refreshHeaderControls();

    if (m_editorConfigFileLabel) m_editorConfigFileLabel->setText(textForKey(QStringLiteral("config_file")));
    if (m_editorConfigFileEdit) m_editorConfigFileEdit->setText(configFilePath());
    if (m_editorReloadButton) m_editorReloadButton->setText(textForKey(QStringLiteral("reload")));
    if (m_editorBrowseButton) m_editorBrowseButton->setText(textForKey(QStringLiteral("browse")));
    if (m_editorAddButton) m_editorAddButton->setText(textForKey(QStringLiteral("add_row")));
    if (m_editorRemoveButton) m_editorRemoveButton->setText(textForKey(QStringLiteral("remove_row")));
    if (m_editorSaveButton) m_editorSaveButton->setText(textForKey(QStringLiteral("save")));

    if (m_configEditorDialog) {
        m_configEditorDialog->setWindowTitle(textForKey(QStringLiteral("config_editor")));
        m_configEditorDialog->resize(1220, 780);
    }

    if (m_statusLabel && m_statusLabel->text().isEmpty()) {
        m_statusLabel->setText(textForKey(QStringLiteral("ready")));
        m_statusLabel->setStyleSheet(UiHelpers::statusStyle(QStringLiteral("info")));
    }

    if (m_editorTabWidget) {
        m_editorTabWidget->setTabText(0, textForKey(QStringLiteral("editor_workstation_section")));
        m_editorTabWidget->setTabText(1, textForKey(QStringLiteral("editor_axis_section")));
        m_editorTabWidget->setTabText(2, textForKey(QStringLiteral("editor_operation_section")));
        m_editorTabWidget->setTabText(3, textForKey(QStringLiteral("editor_move_section")));
    }

    if (m_editorWorkstationTable) {
        QStringList workstationHeaders;
        workstationHeaders << textForKey(QStringLiteral("table_workstation"))
                           << textForKey(QStringLiteral("table_workstation_desc"))
                           << textForKey(QStringLiteral("table_start_command"))
                           << textForKey(QStringLiteral("table_done_variable"))
                           << textForKey(QStringLiteral("table_done_buffer"))
                           << textForKey(QStringLiteral("table_done_value"))
                           << textForKey(QStringLiteral("table_unified_step"))
                           << textForKey(QStringLiteral("table_speed_low"))
                           << textForKey(QStringLiteral("table_speed_high"));
        m_editorWorkstationTable->setHorizontalHeaderLabels(workstationHeaders);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorNameColumn, 140);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorDescriptionColumn, 220);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorStartCommandColumn, 220);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorStartDoneVariableColumn, 180);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorStartDoneBufferColumn, 110);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorStartDoneValueColumn, 110);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorUnifiedStepColumn, 110);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorJogLowColumn, 110);
        m_editorWorkstationTable->setColumnWidth(WorkstationEditorJogHighColumn, 110);
    }

    if (m_editorOperationTable) {
        QStringList operationHeaders;
        operationHeaders << textForKey(QStringLiteral("table_workstation"))
                         << textForKey(QStringLiteral("table_module_key"))
                         << textForKey(QStringLiteral("table_module_name"))
                         << textForKey(QStringLiteral("table_operation_key"))
                         << textForKey(QStringLiteral("table_operation_command"))
                         << textForKey(QStringLiteral("table_done_variable"))
                         << textForKey(QStringLiteral("table_done_buffer"))
                         << textForKey(QStringLiteral("table_done_value"));
        m_editorOperationTable->setHorizontalHeaderLabels(operationHeaders);
        m_editorOperationTable->setColumnWidth(OperationEditorWorkstationColumn, 130);
        m_editorOperationTable->setColumnWidth(OperationEditorModuleKeyColumn, 130);
        m_editorOperationTable->setColumnWidth(OperationEditorModuleNameColumn, 160);
        m_editorOperationTable->setColumnWidth(OperationEditorOperationKeyColumn, 130);
        m_editorOperationTable->setColumnWidth(OperationEditorCommandColumn, 280);
        m_editorOperationTable->setColumnWidth(OperationEditorDoneVariableColumn, 180);
        m_editorOperationTable->setColumnWidth(OperationEditorDoneBufferColumn, 110);
        m_editorOperationTable->setColumnWidth(OperationEditorDoneValueColumn, 110);
    }

    if (m_editorMoveTable) {
        QStringList moveHeaders;
        moveHeaders << textForKey(QStringLiteral("table_workstation"))
                    << textForKey(QStringLiteral("table_step_index"))
                    << textForKey(QStringLiteral("table_axis_name"))
                    << textForKey(QStringLiteral("table_target_position"))
                    << textForKey(QStringLiteral("table_arrival_sensor"));
        m_editorMoveTable->setHorizontalHeaderLabels(moveHeaders);
        m_editorMoveTable->setColumnWidth(MoveEditorWorkstationColumn, 130);
        m_editorMoveTable->setColumnWidth(MoveEditorStepColumn, 90);
        m_editorMoveTable->setColumnWidth(MoveEditorAxisColumn, 120);
        m_editorMoveTable->setColumnWidth(MoveEditorTargetColumn, 130);
        m_editorMoveTable->setColumnWidth(MoveEditorSensorColumn, 260);
    }

    if (m_editorTable) {
        QStringList headers;
        headers << textForKey(QStringLiteral("table_workstation"))
                << textForKey(QStringLiteral("table_workstation_desc"))
                << textForKey(QStringLiteral("table_module_key"))
                << textForKey(QStringLiteral("table_module_name"))
                << textForKey(QStringLiteral("table_module_desc"))
                << textForKey(QStringLiteral("table_axis_name"))
                << textForKey(QStringLiteral("table_axis_number"))
                << textForKey(QStringLiteral("table_speed"))
                << textForKey(QStringLiteral("table_step"))
                << textForKey(QStringLiteral("table_unit"));
        m_editorTable->setHorizontalHeaderLabels(headers);

        m_editorTable->setColumnWidth(WorkstationNameColumn, 140);
        m_editorTable->setColumnWidth(WorkstationDescriptionColumn, 190);
        m_editorTable->setColumnWidth(ModuleKeyColumn, 120);
        m_editorTable->setColumnWidth(ModuleNameColumn, 150);
        m_editorTable->setColumnWidth(ModuleDescriptionColumn, 190);
        m_editorTable->setColumnWidth(AxisNameColumn, 120);
        m_editorTable->setColumnWidth(AxisNumberColumn, 90);
        m_editorTable->setColumnWidth(AxisSpeedColumn, 110);
        m_editorTable->setColumnWidth(AxisStepColumn, 110);
        m_editorTable->setColumnWidth(AxisUnitColumn, 90);
        m_editorTable->setColumnHidden(AxisSpeedColumn, true);
        m_editorTable->setColumnHidden(AxisStepColumn, true);
    }
}

void WorkstationCalibrationPage::refreshHeaderControls()
{
    const WorkstationCalibration* calibration = currentCalibration();
    const bool hasCalibration = calibration != nullptr;
    const bool showWorkstationSwitch = m_workstationCombo && m_workstationCombo->count() > 1;

    if (m_workstationSwitchContainer) {
        m_workstationSwitchContainer->setVisible(showWorkstationSwitch);
    }
    if (m_workstationLabel) {
        m_workstationLabel->setVisible(showWorkstationSwitch);
    }
    if (m_workstationCombo) {
        m_workstationCombo->setVisible(showWorkstationSwitch);
        m_workstationCombo->setEnabled(showWorkstationSwitch);
    }

    if (m_headerStepSpin) {
        QSignalBlocker blocker(m_headerStepSpin);
        m_headerStepSpin->setEnabled(hasCalibration);
        m_headerStepSpin->setValue(hasCalibration ? calibration->unifiedStep : 1.0);
    }

    if (m_headerSettingsButton) {
        m_headerSettingsButton->setText(textForKey(QStringLiteral("settings")));
        m_headerSettingsButton->setStyleSheet(UiHelpers::secondaryButtonStyle());
    }
}

void WorkstationCalibrationPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_initialConfigLoaded) {
        m_initialConfigLoaded = true;
        QTimer::singleShot(0, this, [this]() {
            reloadConfig();
        });
    }
    updatePositionRefreshState();
}

void WorkstationCalibrationPage::hideEvent(QHideEvent* event)
{
    updatePositionRefreshState();
    QWidget::hideEvent(event);
}

void WorkstationCalibrationPage::updatePositionRefreshState()
{
    if (!m_positionRefreshTimer) {
        return;
    }

    const bool canRefresh = isVisible()
        && m_initialConfigLoaded
        && !m_selectedWorkstationName.isEmpty()
        && !m_axisPositionDisplays.isEmpty();

    if (canRefresh) {
        if (!m_positionRefreshTimer->isActive()) {
            m_positionRefreshTimer->start();
        }
    } else {
        m_positionRefreshTimer->stop();
    }
}

void WorkstationCalibrationPage::rebuildCalibrationView()
{
    if (m_positionRefreshTimer) {
        m_positionRefreshTimer->stop();
    }

    if (m_scrollArea) {
        m_scrollArea->setUpdatesEnabled(false);
    }

    clearContentLayout();

    int currentIndex = -1;
    if (!m_selectedWorkstationName.isEmpty()) {
        for (int i = 0; i < m_calibrations.size(); ++i) {
            if (m_calibrations.at(i).calibrationName == m_selectedWorkstationName) {
                currentIndex = i;
                break;
            }
        }
    }
    if (currentIndex < 0 || currentIndex >= m_calibrations.size()) {
        auto* emptyLabel = new QLabel(textForKey(QStringLiteral("empty")), m_contentWidget);
        emptyLabel->setStyleSheet(QStringLiteral("font-size:14px;color:#5f6b76;"));
        m_contentLayout->addWidget(emptyLabel);
        m_contentLayout->addStretch();
        if (m_scrollArea) {
            m_scrollArea->setUpdatesEnabled(true);
        }
        updatePositionRefreshState();
        return;
    }

    const WorkstationCalibration& calibration = m_calibrations.at(currentIndex);
    if (!calibration.moveSequence.isEmpty()) {
        m_contentLayout->addWidget(createMoveSequenceCard(calibration));
    }

    auto* modulesContainer = new QWidget(m_contentWidget);
    auto* modulesGrid = new QGridLayout(modulesContainer);
    modulesGrid->setContentsMargins(0, 0, 0, 0);
    modulesGrid->setHorizontalSpacing(12);
    modulesGrid->setVerticalSpacing(12);

    QVector<CalibrationModule> displayModules = calibration.modules;
    auto moduleOrder = [](const CalibrationModule& module) {
        if (isForkModule(module)) {
            return 0;
        }
        if (isAlignerModule(module)) {
            return 1;
        }
        if (isEdgeChuckModule(module)) {
            return 2;
        }
        if (isMainChuckModule(module)) {
            return 3;
        }
        if (isRobotArmModule(module)) {
            return 4;
        }
        return 9;
    };
    std::stable_sort(displayModules.begin(), displayModules.end(), [&](const CalibrationModule& left, const CalibrationModule& right) {
        return moduleOrder(left) < moduleOrder(right);
    });

    for (int moduleIndex = 0; moduleIndex < displayModules.size(); ++moduleIndex) {
        const CalibrationModule& module = displayModules.at(moduleIndex);
        const int row = moduleIndex / 2;
        const int column = moduleIndex % 2;
        modulesGrid->addWidget(createModuleWidget(module, false), row, column);
    }
    modulesGrid->setColumnStretch(0, 1);
    modulesGrid->setColumnStretch(1, 1);
    m_contentLayout->addWidget(modulesContainer);

    m_contentLayout->addStretch();

    if (m_scrollArea) {
        m_scrollArea->setUpdatesEnabled(true);
    }
    updatePositionRefreshState();
}

void WorkstationCalibrationPage::populateEditorTable()
{
    if (!m_editorTable || !m_editorWorkstationTable || !m_editorMoveTable || !m_editorOperationTable) {
        return;
    }

    QSignalBlocker blocker(m_editorTable);
    QSignalBlocker workstationBlocker(m_editorWorkstationTable);
    QSignalBlocker moveBlocker(m_editorMoveTable);
    QSignalBlocker operationBlocker(m_editorOperationTable);

    m_editorWorkstationTable->clearContents();
    m_editorWorkstationTable->setRowCount(m_calibrations.size());

    int workstationRow = 0;
    int moveRowCount = 0;
    int operationRowCount = 0;
    for (const WorkstationCalibration& calibration : m_calibrations) {
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorNameColumn, makeItem(calibration.calibrationName));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorDescriptionColumn, makeItem(calibration.description));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorStartCommandColumn, makeItem(calibration.startPositionCommand));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorStartDoneVariableColumn, makeItem(calibration.startPositionDoneVariable));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorStartDoneBufferColumn, makeItem(QString::number(calibration.startPositionDoneBuffer)));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorStartDoneValueColumn, makeItem(QString::number(calibration.startPositionDoneValue)));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorUnifiedStepColumn, makeItem(QString::number(calibration.unifiedStep, 'f', 3)));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorJogLowColumn, makeItem(QString::number(calibration.jogSpeedLow, 'f', 3)));
        m_editorWorkstationTable->setItem(workstationRow, WorkstationEditorJogHighColumn, makeItem(QString::number(calibration.jogSpeedHigh, 'f', 3)));
        ++workstationRow;

        moveRowCount += calibration.moveSequence.size();
        for (const CalibrationModule& module : calibration.modules) {
            operationRowCount += module.operations.size();
        }
    }

    m_editorMoveTable->clearContents();
    m_editorMoveTable->setRowCount(moveRowCount);
    int moveRow = 0;
    for (const WorkstationCalibration& calibration : m_calibrations) {
        for (int index = 0; index < calibration.moveSequence.size(); ++index) {
            const CalibrationMoveStep& step = calibration.moveSequence.at(index);
            m_editorMoveTable->setItem(moveRow, MoveEditorWorkstationColumn, makeItem(calibration.calibrationName));
            m_editorMoveTable->setItem(moveRow, MoveEditorStepColumn, makeItem(QString::number(index + 1)));
            m_editorMoveTable->setItem(moveRow, MoveEditorAxisColumn, makeItem(step.axisName));
            m_editorMoveTable->setItem(moveRow, MoveEditorTargetColumn, makeItem(QString::number(step.targetPosition, 'f', 3)));
            m_editorMoveTable->setItem(moveRow, MoveEditorSensorColumn, makeItem(step.arrivalSensorVariable));
            ++moveRow;
        }
    }

    m_editorOperationTable->clearContents();
    m_editorOperationTable->setRowCount(operationRowCount);
    int operationRow = 0;
    for (const WorkstationCalibration& calibration : m_calibrations) {
        for (const CalibrationModule& module : calibration.modules) {
            for (auto it = module.operations.cbegin(); it != module.operations.cend(); ++it) {
                m_editorOperationTable->setItem(operationRow, OperationEditorWorkstationColumn, makeItem(calibration.calibrationName));
                m_editorOperationTable->setItem(operationRow, OperationEditorModuleKeyColumn, makeItem(module.key));
                m_editorOperationTable->setItem(operationRow, OperationEditorModuleNameColumn, makeItem(module.name));
                m_editorOperationTable->setCellWidget(operationRow, OperationEditorOperationKeyColumn, createOperationKeyCombo(m_editorOperationTable, it.key()));
                m_editorOperationTable->setItem(operationRow, OperationEditorCommandColumn, makeItem(it.value().command));
                m_editorOperationTable->setItem(operationRow, OperationEditorDoneVariableColumn, makeItem(it.value().doneVariable));
                m_editorOperationTable->setItem(operationRow, OperationEditorDoneBufferColumn, makeItem(QString::number(it.value().doneBuffer)));
                m_editorOperationTable->setItem(operationRow, OperationEditorDoneValueColumn, makeItem(QString::number(it.value().doneValue)));
                ++operationRow;
            }
        }
    }

    int rowCount = 0;
    for (const WorkstationCalibration& calibration : m_calibrations) {
        for (const CalibrationModule& module : calibration.modules) {
            rowCount += module.axes.size();
        }
    }

    m_editorTable->clearContents();
    m_editorTable->setRowCount(rowCount);

    int row = 0;
    for (const WorkstationCalibration& calibration : m_calibrations) {
        for (const CalibrationModule& module : calibration.modules) {
            for (const CalibrationAxis& axis : module.axes) {
                m_editorTable->setItem(row, WorkstationNameColumn, makeItem(calibration.calibrationName));
                m_editorTable->setItem(row, WorkstationDescriptionColumn, makeItem(calibration.description));
                m_editorTable->setItem(row, ModuleKeyColumn, makeItem(module.key));
                m_editorTable->setItem(row, ModuleNameColumn, makeItem(module.name));
                m_editorTable->setItem(row, ModuleDescriptionColumn, makeItem(module.description));
                m_editorTable->setItem(row, AxisNameColumn, makeItem(axis.name));
                m_editorTable->setItem(row, AxisNumberColumn, makeItem(QString::number(axis.axisNumber)));
                m_editorTable->setItem(row, AxisSpeedColumn, makeItem(QString::number(axis.defaultSpeed, 'f', 3)));
                m_editorTable->setItem(row, AxisStepColumn, makeItem(QString::number(axis.defaultStep, 'f', 3)));
                m_editorTable->setItem(row, AxisUnitColumn, makeItem(axis.unit));
                ++row;
            }
        }
    }

    if (m_editorConfigFileEdit) {
        m_editorConfigFileEdit->setText(configFilePath());
    }
}

void WorkstationCalibrationPage::updateWatcher()
{
    if (!m_configWatcher) {
        return;
    }

    const QStringList watchedFiles = m_configWatcher->files();
    for (const QString& filePath : watchedFiles) {
        m_configWatcher->removePath(filePath);
    }

    if (QFileInfo::exists(configFilePath())) {
        m_configWatcher->addPath(configFilePath());
    }
}

void WorkstationCalibrationPage::clearContentLayout()
{
    if (!m_scrollArea) {
        return;
    }

    m_axisPositionDisplays.clear();
    m_moveStepButtons.clear();
    m_startPositionButtons.clear();
    m_moveSequenceUnifiedStepSpins.clear();

    if (QWidget* previousWidget = m_scrollArea->takeWidget()) {
        previousWidget->hide();
        previousWidget->setParent(nullptr);
        previousWidget->deleteLater();
    }

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(12);
    m_scrollArea->setWidget(m_contentWidget);

    if (m_positionStatusLayout) {
        clearLayoutWidgets(m_positionStatusLayout);
    }
}

void WorkstationCalibrationPage::showStatusMessage(const QString& text, bool isError)
{
    setStatusMessage(text, isError, true);
}

void WorkstationCalibrationPage::logMessage(const QString& message)
{
    appendLogMessage(message);
}

void WorkstationCalibrationPage::showFailureDialog(const QString& title, const QString& message)
{
    Q_UNUSED(title);
    Q_UNUSED(message);
}

void WorkstationCalibrationPage::setCalibrationInteractionEnabled(bool enabled)
{
    m_calibrationInteractionEnabled = enabled;
    const auto buttons = findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (!button) {
            continue;
        }
        if (button->property("settingsLauncher").toBool()) {
            continue;
        }
        button->setEnabled(enabled);
    }
    if (m_workstationCombo) {
        m_workstationCombo->setEnabled(enabled);
    }
}

bool WorkstationCalibrationPage::collectCalibrationsFromEditor(QVector<WorkstationCalibration>* calibrations, QString* errorMessage) const
{
    if (!m_editorTable || !m_editorWorkstationTable || !m_editorMoveTable || !m_editorOperationTable || !calibrations) {
        return false;
    }

    calibrations->clear();

    QMap<QString, int> calibrationIndexByName;
    QMap<QString, int> moduleIndexByCompositeKey;

    for (int row = 0; row < m_editorWorkstationTable->rowCount(); ++row) {
        const auto textAt = [&](int column) -> QString {
            QTableWidgetItem* item = m_editorWorkstationTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        const QString workstationName = textAt(WorkstationEditorNameColumn);
        if (workstationName.isEmpty()) {
            if (errorMessage) {
                *errorMessage = trText(m_languageCode,
                                       QStringLiteral("Workstation name is required in row %1").arg(row + 1),
                                       QStringLiteral("\u7b2c %1 \u884c\u5de5\u4f4d\u540d\u79f0\u4e0d\u80fd\u4e3a\u7a7a").arg(row + 1));
            }
            return false;
        }

        bool unifiedStepOk = false;
        bool startDoneValueOk = false;
        bool startDoneBufferOk = false;
        bool jogLowOk = false;
        bool jogHighOk = false;
        const int startDoneBuffer = textAt(WorkstationEditorStartDoneBufferColumn).toInt(&startDoneBufferOk);
        const int startDoneValue = textAt(WorkstationEditorStartDoneValueColumn).toInt(&startDoneValueOk);
        const double unifiedStep = textAt(WorkstationEditorUnifiedStepColumn).toDouble(&unifiedStepOk);
        const double jogLow = textAt(WorkstationEditorJogLowColumn).toDouble(&jogLowOk);
        const double jogHigh = textAt(WorkstationEditorJogHighColumn).toDouble(&jogHighOk);
        if (!startDoneBufferOk || !startDoneValueOk || !unifiedStepOk || !jogLowOk || !jogHighOk) {
            if (errorMessage) {
                *errorMessage = trText(m_languageCode,
                                       QStringLiteral("Invalid workstation numeric values in row %1").arg(row + 1),
                                       QStringLiteral("\u7b2c %1 \u884c\u5de5\u4f4d\u6570\u503c\u65e0\u6548").arg(row + 1));
            }
            return false;
        }

        WorkstationCalibration calibration;
        calibration.calibrationName = workstationName;
        calibration.description = textAt(WorkstationEditorDescriptionColumn);
        calibration.startPositionCommand = textAt(WorkstationEditorStartCommandColumn);
        calibration.startPositionDoneVariable = textAt(WorkstationEditorStartDoneVariableColumn);
        calibration.startPositionDoneBuffer = startDoneBuffer;
        calibration.startPositionDoneValue = startDoneValue;
        calibration.unifiedStep = unifiedStep;
        calibration.jogSpeedLow = jogLow;
        calibration.jogSpeedHigh = jogHigh;
        calibrationIndexByName.insert(workstationName, calibrations->size());
        calibrations->push_back(calibration);
    }

    for (int row = 0; row < m_editorTable->rowCount(); ++row) {
        const auto textAt = [&](int column) -> QString {
            QTableWidgetItem* item = m_editorTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        const QString workstationName = textAt(WorkstationNameColumn);
        const QString workstationDescription = textAt(WorkstationDescriptionColumn);
        const QString moduleKey = textAt(ModuleKeyColumn);
        const QString moduleName = textAt(ModuleNameColumn);
        const QString moduleDescription = textAt(ModuleDescriptionColumn);
        const QString axisName = textAt(AxisNameColumn);
        const QString axisNumberText = textAt(AxisNumberColumn);
        const QString axisSpeedText = textAt(AxisSpeedColumn);
        const QString axisStepText = textAt(AxisStepColumn);
        const QString axisUnitText = textAt(AxisUnitColumn);

        if (workstationName.isEmpty() || moduleKey.isEmpty() || axisName.isEmpty()) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("missing_required_fields")).arg(row + 1);
            }
            return false;
        }

        bool axisNumberOk = false;
        bool speedOk = false;
        bool stepOk = false;
        const int axisNumber = axisNumberText.toInt(&axisNumberOk);
        const double defaultSpeed = axisSpeedText.toDouble(&speedOk);
        const double defaultStep = axisStepText.toDouble(&stepOk);

        if (!axisNumberOk || axisNumber < 0 || !speedOk || !stepOk) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("invalid_numeric")).arg(row + 1);
            }
            return false;
        }

        const int calibrationIndex = calibrationIndexByName.value(workstationName, -1);
        if (calibrationIndex < 0) {
            if (errorMessage) {
                *errorMessage = trText(m_languageCode,
                                       QStringLiteral("Axis row %1 references unknown workstation %2").arg(row + 1).arg(workstationName),
                                       QStringLiteral("\u7b2c %1 \u884c\u8f74\u6570\u636e\u5f15\u7528\u4e86\u672a\u77e5\u5de5\u4f4d %2").arg(row + 1).arg(workstationName));
            }
            return false;
        }

        WorkstationCalibration& calibration = (*calibrations)[calibrationIndex];
        calibration.description = workstationDescription;

        const QString moduleCompositeKey = workstationName + QChar(0x1f) + moduleKey;
        int moduleIndex = moduleIndexByCompositeKey.value(moduleCompositeKey, -1);
        if (moduleIndex < 0) {
            CalibrationModule module;
            module.key = moduleKey;
            module.name = moduleName.isEmpty() ? moduleKey : moduleName;
            module.description = moduleDescription;
            moduleIndex = calibration.modules.size();
            calibration.modules.push_back(module);
            moduleIndexByCompositeKey.insert(moduleCompositeKey, moduleIndex);
        }

        CalibrationModule& module = calibration.modules[moduleIndex];
        module.name = moduleName.isEmpty() ? moduleKey : moduleName;
        module.description = moduleDescription;

        CalibrationAxis axis;
        axis.name = axisName;
        axis.axisNumber = axisNumber;
        axis.defaultSpeed = defaultSpeed;
        axis.defaultStep = defaultStep;
        axis.unit = axisUnitText.isEmpty() ? QStringLiteral("mm") : axisUnitText;
        module.axes.push_back(axis);
    }

    for (int row = 0; row < m_editorOperationTable->rowCount(); ++row) {
        const auto textAt = [&](int column) -> QString {
            QTableWidgetItem* item = m_editorOperationTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        const QString workstationName = textAt(OperationEditorWorkstationColumn);
        const QString moduleKey = textAt(OperationEditorModuleKeyColumn);
        QString operationKey;
        if (QWidget* widget = m_editorOperationTable->cellWidget(row, OperationEditorOperationKeyColumn)) {
            if (auto* combo = qobject_cast<QComboBox*>(widget)) {
                operationKey = normalizeOperationKey(combo->currentText());
            }
        }
        if (operationKey.isEmpty()) {
            operationKey = normalizeOperationKey(textAt(OperationEditorOperationKeyColumn));
        }
        const QString moduleName = textAt(OperationEditorModuleNameColumn);
        const QString command = textAt(OperationEditorCommandColumn);
        const QString doneVariable = textAt(OperationEditorDoneVariableColumn);
        bool doneBufferOk = false;
        bool doneValueOk = false;
        const int doneBuffer = textAt(OperationEditorDoneBufferColumn).toInt(&doneBufferOk);
        const int doneValue = textAt(OperationEditorDoneValueColumn).toInt(&doneValueOk);
        if (workstationName.isEmpty() || moduleKey.isEmpty() || operationKey.isEmpty()) {
            continue;
        }
        if (!doneBufferOk || !doneValueOk) {
            if (errorMessage) {
                *errorMessage = trText(m_languageCode,
                                       QStringLiteral("Invalid operation done value in row %1").arg(row + 1),
                                       QStringLiteral("\u7b2c %1 \u884c\u64cd\u4f5c\u5b8c\u6210\u503c\u65e0\u6548").arg(row + 1));
            }
            return false;
        }

        const int calibrationIndex = calibrationIndexByName.value(workstationName, -1);
        if (calibrationIndex < 0) {
            continue;
        }

        WorkstationCalibration& calibration = (*calibrations)[calibrationIndex];
        CalibrationModule* matchedModule = nullptr;
        for (CalibrationModule& module : calibration.modules) {
            if (module.key == moduleKey) {
                matchedModule = &module;
                break;
            }
        }
        if (!matchedModule) {
            CalibrationModule module;
            module.key = moduleKey;
            module.name = moduleName.isEmpty() ? moduleKey : moduleName;
            calibration.modules.push_back(module);
            matchedModule = &calibration.modules.last();
        }
        matchedModule->name = moduleName.isEmpty() ? moduleKey : moduleName;
        CalibrationCommandAction action;
        action.command = command;
        action.doneVariable = doneVariable;
        action.doneBuffer = doneBuffer;
        action.doneValue = doneValue;
        matchedModule->operations.insert(operationKey, action);
    }

    for (int row = 0; row < m_editorMoveTable->rowCount(); ++row) {
        const auto textAt = [&](int column) -> QString {
            QTableWidgetItem* item = m_editorMoveTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        const QString workstationName = textAt(MoveEditorWorkstationColumn);
        const QString axisName = textAt(MoveEditorAxisColumn);
        if (workstationName.isEmpty() || axisName.isEmpty()) {
            continue;
        }

        bool stepIndexOk = false;
        bool targetOk = false;
        const int stepIndex = textAt(MoveEditorStepColumn).toInt(&stepIndexOk);
        const double targetPosition = textAt(MoveEditorTargetColumn).toDouble(&targetOk);
        if (!stepIndexOk || stepIndex <= 0 || !targetOk) {
            if (errorMessage) {
                *errorMessage = trText(m_languageCode,
                                       QStringLiteral("Invalid move-sequence row %1").arg(row + 1),
                                       QStringLiteral("\u79fb\u52a8\u987a\u5e8f\u7b2c %1 \u884c\u65e0\u6548").arg(row + 1));
            }
            return false;
        }

        const int calibrationIndex = calibrationIndexByName.value(workstationName, -1);
        if (calibrationIndex < 0) {
            continue;
        }

        WorkstationCalibration& calibration = (*calibrations)[calibrationIndex];
        if (calibration.moveSequence.size() < stepIndex) {
            calibration.moveSequence.resize(stepIndex);
        }
        CalibrationMoveStep& step = calibration.moveSequence[stepIndex - 1];
        step.axisName = axisName;
        step.targetPosition = targetPosition;
        step.arrivalSensorVariable = textAt(MoveEditorSensorColumn);
    }

    if (calibrations->isEmpty()) {
        if (errorMessage) {
            *errorMessage = textForKey(QStringLiteral("empty_save"));
        }
        return false;
    }

    return true;
}

QString WorkstationCalibrationPage::textForKey(const QString& key) const
{
    if (key == QStringLiteral("ready")) {
        return trText(m_languageCode, QStringLiteral("Ready"), QStringLiteral("\u5c31\u7eea"));
    }
    if (key == QStringLiteral("runtime_log")) {
        return trText(m_languageCode, QStringLiteral("Runtime Log"), QStringLiteral("\u8fd0\u884c\u65e5\u5fd7"));
    }
    if (key == QStringLiteral("settings")) {
        return trText(m_languageCode, QStringLiteral("Settings"), QStringLiteral("\u8bbe\u7f6e"));
    }
    if (key == QStringLiteral("title")) {
        return trText(m_languageCode, QStringLiteral("Station Calibration"), QStringLiteral("\u5de5\u4f4d\u6807\u5b9a"));
    }
    if (key == QStringLiteral("desc")) {
        return trText(m_languageCode,
                      QStringLiteral("Edit workstation calibration definitions, save them back to JSON, and hot reload the page when the file changes."),
                      QStringLiteral("\u652f\u6301\u7f16\u8f91\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e\uff0c\u4fdd\u5b58\u5230 JSON\uff0c\u5e76\u5728\u914d\u7f6e\u6587\u4ef6\u53d1\u751f\u53d8\u66f4\u65f6\u81ea\u52a8\u70ed\u52a0\u8f7d\u754c\u9762\u3002"));
    }
    if (key == QStringLiteral("workstation")) {
        return trText(m_languageCode, QStringLiteral("Workstation"), QStringLiteral("\u5de5\u4f4d"));
    }
    if (key == QStringLiteral("reload")) {
        return trText(m_languageCode, QStringLiteral("Reload Config"), QStringLiteral("\u91cd\u65b0\u52a0\u8f7d\u914d\u7f6e"));
    }
    if (key == QStringLiteral("config_editor")) {
        return trText(m_languageCode, QStringLiteral("Calibration Config"), QStringLiteral("\u6807\u5b9a\u914d\u7f6e"));
    }
    if (key == QStringLiteral("config_editor_title")) {
        return trText(m_languageCode,
                      QStringLiteral("Workstation Calibration Config Editor"),
                      QStringLiteral("\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e\u7f16\u8f91\u5668"));
    }
    if (key == QStringLiteral("config_path")) {
        return trText(m_languageCode, QStringLiteral("Config: %1"), QStringLiteral("\u914d\u7f6e\u6587\u4ef6\uff1a%1"));
    }
    if (key == QStringLiteral("config_file")) {
        return trText(m_languageCode, QStringLiteral("Config File"), QStringLiteral("\u914d\u7f6e\u6587\u4ef6"));
    }
    if (key == QStringLiteral("browse")) {
        return trText(m_languageCode, QStringLiteral("Browse"), QStringLiteral("\u6d4f\u89c8"));
    }
    if (key == QStringLiteral("add_axis")) {
        return trText(m_languageCode, QStringLiteral("Add Axis"), QStringLiteral("\u65b0\u589e\u8f74"));
    }
    if (key == QStringLiteral("remove_axis")) {
        return trText(m_languageCode, QStringLiteral("Remove Axis"), QStringLiteral("\u5220\u9664\u8f74"));
    }
    if (key == QStringLiteral("add_row")) {
        return trText(m_languageCode, QStringLiteral("Add"), QStringLiteral("\u65b0\u589e"));
    }
    if (key == QStringLiteral("remove_row")) {
        return trText(m_languageCode, QStringLiteral("Delete"), QStringLiteral("\u5220\u9664"));
    }
    if (key == QStringLiteral("save")) {
        return trText(m_languageCode, QStringLiteral("Save"), QStringLiteral("\u4fdd\u5b58"));
    }
    if (key == QStringLiteral("editor_workstation_section")) {
        return trText(m_languageCode, QStringLiteral("Workstation Fields"), QStringLiteral("\u5de5\u4f4d\u57fa\u672c\u5b57\u6bb5"));
    }
    if (key == QStringLiteral("editor_json_section")) {
        return trText(m_languageCode, QStringLiteral("Full JSON"), QStringLiteral("\u5b8c\u6574 JSON"));
    }
    if (key == QStringLiteral("editor_axis_section")) {
        return trText(m_languageCode, QStringLiteral("Axis Config Table"), QStringLiteral("\u8f74\u914d\u7f6e\u8868\u683c"));
    }
    if (key == QStringLiteral("editor_move_section")) {
        return trText(m_languageCode, QStringLiteral("Move Sequence Fields"), QStringLiteral("\u79fb\u52a8\u987a\u5e8f\u5b57\u6bb5"));
    }
    if (key == QStringLiteral("editor_operation_section")) {
        return trText(m_languageCode, QStringLiteral("Module Operations"), QStringLiteral("\u6a21\u5757\u64cd\u4f5c\u547d\u4ee4"));
    }
    if (key == QStringLiteral("loaded")) {
        return trText(m_languageCode,
                      QStringLiteral("Loaded %1 workstation calibration definitions"),
                      QStringLiteral("\u5df2\u52a0\u8f7d %1 \u4e2a\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e"));
    }
    if (key == QStringLiteral("empty")) {
        return trText(m_languageCode,
                      QStringLiteral("No workstation calibration data is available"),
                      QStringLiteral("\u5f53\u524d\u6ca1\u6709\u53ef\u7528\u7684\u5de5\u4f4d\u6807\u5b9a\u6570\u636e"));
    }
    if (key == QStringLiteral("position")) {
        return trText(m_languageCode, QStringLiteral("Pos"), QStringLiteral("\u4f4d\u7f6e"));
    }
    if (key == QStringLiteral("speed")) {
        return trText(m_languageCode, QStringLiteral("Speed"), QStringLiteral("\u901f\u5ea6"));
    }
    if (key == QStringLiteral("step")) {
        return trText(m_languageCode, QStringLiteral("Step"), QStringLiteral("\u6b65\u957f"));
    }
    if (key == QStringLiteral("axis")) {
        return trText(m_languageCode, QStringLiteral("Axis"), QStringLiteral("\u8f74"));
    }
    if (key == QStringLiteral("move_sequence")) {
        return trText(m_languageCode, QStringLiteral("Move Sequence"), QStringLiteral("\u79fb\u52a8\u987a\u5e8f"));
    }
    if (key == QStringLiteral("move_sequence_desc")) {
        return trText(m_languageCode,
                      QStringLiteral("Click a step below to move the corresponding axis directly to its configured target position."),
                      QStringLiteral("\u70b9\u51fb\u4e0b\u65b9\u6b65\u9aa4\uff0c\u53ef\u76f4\u63a5\u5c06\u5bf9\u5e94\u8f74\u79fb\u52a8\u5230\u914d\u7f6e\u7684\u76ee\u6807\u4f4d\u7f6e\u3002"));
    }
    if (key == QStringLiteral("save_current_position")) {
        return trText(m_languageCode, QStringLiteral("Save Current Position"), QStringLiteral("\u4fdd\u5b58\u5f53\u524d\u4f4d\u7f6e"));
    }
    if (key == QStringLiteral("start_position")) {
        return trText(m_languageCode, QStringLiteral("Start"), QStringLiteral("\u8d77\u59cb\u4f4d"));
    }
    if (key == QStringLiteral("module_empty")) {
        return trText(m_languageCode,
                      QStringLiteral("No axis configuration was found for this module"),
                      QStringLiteral("\u8be5\u6a21\u5757\u672a\u627e\u5230\u8f74\u914d\u7f6e"));
    }
    if (key == QStringLiteral("save_success")) {
        return trText(m_languageCode,
                      QStringLiteral("Calibration config saved successfully"),
                      QStringLiteral("\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e\u4fdd\u5b58\u6210\u529f"));
    }
    if (key == QStringLiteral("reload_success")) {
        return trText(m_languageCode,
                      QStringLiteral("Calibration config hot reloaded"),
                      QStringLiteral("\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e\u5df2\u70ed\u52a0\u8f7d"));
    }
    if (key == QStringLiteral("switch_success")) {
        return trText(m_languageCode,
                      QStringLiteral("Switched calibration config: %1"),
                      QStringLiteral("\u5df2\u5207\u6362\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e\uff1a%1"));
    }
    if (key == QStringLiteral("missing_required_fields")) {
        return trText(m_languageCode,
                      QStringLiteral("Workstation, module key, and axis name are required on row %1"),
                      QStringLiteral("\u7b2c %1 \u884c\u7f3a\u5c11\u5fc5\u586b\u5b57\u6bb5\uff1a\u5de5\u4f4d\u3001\u6a21\u5757\u6807\u8bc6\u3001\u8f74\u540d\u79f0"));
    }
    if (key == QStringLiteral("invalid_numeric")) {
        return trText(m_languageCode,
                      QStringLiteral("Axis number, speed, or step is invalid on row %1"),
                      QStringLiteral("\u7b2c %1 \u884c\u7684\u8f74\u53f7\u3001\u901f\u5ea6\u6216\u6b65\u957f\u65e0\u6548"));
    }
    if (key == QStringLiteral("empty_save")) {
        return trText(m_languageCode,
                      QStringLiteral("No calibration rows are available to save"),
                      QStringLiteral("\u5f53\u524d\u6ca1\u6709\u53ef\u4fdd\u5b58\u7684\u6807\u5b9a\u8f74\u914d\u7f6e"));
    }
    if (key == QStringLiteral("table_workstation")) {
        return trText(m_languageCode, QStringLiteral("Workstation"), QStringLiteral("\u5de5\u4f4d"));
    }
    if (key == QStringLiteral("table_workstation_desc")) {
        return trText(m_languageCode, QStringLiteral("Workstation Desc"), QStringLiteral("\u5de5\u4f4d\u8bf4\u660e"));
    }
    if (key == QStringLiteral("table_module_key")) {
        return trText(m_languageCode, QStringLiteral("Module Key"), QStringLiteral("\u6a21\u5757\u6807\u8bc6"));
    }
    if (key == QStringLiteral("table_module_name")) {
        return trText(m_languageCode, QStringLiteral("Module Name"), QStringLiteral("\u6a21\u5757\u540d\u79f0"));
    }
    if (key == QStringLiteral("table_module_desc")) {
        return trText(m_languageCode, QStringLiteral("Module Desc"), QStringLiteral("\u6a21\u5757\u8bf4\u660e"));
    }
    if (key == QStringLiteral("table_axis_name")) {
        return trText(m_languageCode, QStringLiteral("Axis Name"), QStringLiteral("\u8f74\u540d\u79f0"));
    }
    if (key == QStringLiteral("table_axis_number")) {
        return trText(m_languageCode, QStringLiteral("Axis No."), QStringLiteral("\u8f74\u53f7"));
    }
    if (key == QStringLiteral("table_speed")) {
        return trText(m_languageCode, QStringLiteral("Default Speed"), QStringLiteral("\u9ed8\u8ba4\u901f\u5ea6"));
    }
    if (key == QStringLiteral("table_step")) {
        return trText(m_languageCode, QStringLiteral("Default Step"), QStringLiteral("\u9ed8\u8ba4\u6b65\u957f"));
    }
    if (key == QStringLiteral("table_unit")) {
        return trText(m_languageCode, QStringLiteral("Unit"), QStringLiteral("\u5355\u4f4d"));
    }
    if (key == QStringLiteral("table_start_command")) {
        return trText(m_languageCode, QStringLiteral("Start Command"), QStringLiteral("\u8d77\u59cb\u4f4d\u547d\u4ee4"));
    }
    if (key == QStringLiteral("table_unified_step")) {
        return trText(m_languageCode, QStringLiteral("Unified Step"), QStringLiteral("\u7edf\u4e00\u6b65\u957f"));
    }
    if (key == QStringLiteral("table_speed_low")) {
        return trText(m_languageCode, QStringLiteral("Jog Low Speed"), QStringLiteral("\u4f4e\u901f"));
    }
    if (key == QStringLiteral("table_speed_high")) {
        return trText(m_languageCode, QStringLiteral("Jog High Speed"), QStringLiteral("\u9ad8\u901f"));
    }
    if (key == QStringLiteral("table_step_index")) {
        return trText(m_languageCode, QStringLiteral("Step No."), QStringLiteral("\u6b65\u9aa4\u53f7"));
    }
    if (key == QStringLiteral("table_target_position")) {
        return trText(m_languageCode, QStringLiteral("Target Position"), QStringLiteral("\u76ee\u6807\u4f4d\u7f6e"));
    }
    if (key == QStringLiteral("table_arrival_sensor")) {
        return trText(m_languageCode, QStringLiteral("Arrival Sensor"), QStringLiteral("\u5230\u4f4d\u4f20\u611f\u5668"));
    }
    if (key == QStringLiteral("table_operation_key")) {
        return trText(m_languageCode, QStringLiteral("Operation Key"), QStringLiteral("\u64cd\u4f5c\u6807\u8bc6"));
    }
    if (key == QStringLiteral("table_operation_command")) {
        return trText(m_languageCode, QStringLiteral("Operation Command"), QStringLiteral("\u64cd\u4f5c\u547d\u4ee4"));
    }
    if (key == QStringLiteral("table_done_variable")) {
        return trText(m_languageCode, QStringLiteral("Done Variable"), QStringLiteral("\u5b8c\u6210\u53d8\u91cf"));
    }
    if (key == QStringLiteral("table_done_buffer")) {
        return trText(m_languageCode, QStringLiteral("Done Buffer"), QStringLiteral("\u5b8c\u6210Buffer"));
    }
    if (key == QStringLiteral("table_done_value")) {
        return trText(m_languageCode, QStringLiteral("Done Value"), QStringLiteral("\u5b8c\u6210\u503c"));
    }
    if (key == QStringLiteral("jog_settings")) {
        return trText(m_languageCode, QStringLiteral("Jog Settings"), QStringLiteral("\u70b9\u52a8\u53c2\u6570"));
    }
    if (key == QStringLiteral("speed_mode")) {
        return trText(m_languageCode, QStringLiteral("Speed Mode"), QStringLiteral("\u901f\u5ea6\u6863\u4f4d"));
    }
    if (key == QStringLiteral("speed_low")) {
        return trText(m_languageCode, QStringLiteral("Low"), QStringLiteral("\u4f4e\u901f"));
    }
    if (key == QStringLiteral("speed_high")) {
        return trText(m_languageCode, QStringLiteral("High"), QStringLiteral("\u9ad8\u901f"));
    }
    if (key == QStringLiteral("arrival_on")) {
        return trText(m_languageCode, QStringLiteral("Arrival sensor ON"), QStringLiteral("\u5230\u4f4d\u4f20\u611f\u5668\u5df2\u89e6\u53d1"));
    }
    if (key == QStringLiteral("arrival_off")) {
        return trText(m_languageCode, QStringLiteral("Arrival sensor OFF"), QStringLiteral("\u5230\u4f4d\u4f20\u611f\u5668\u672a\u89e6\u53d1"));
    }

    return key;
}

QString WorkstationCalibrationPage::configFilePath() const
{
    return m_configFilePath;
}

QString WorkstationCalibrationPage::axisKey(const QString& workstationName, const QString& moduleKey, const CalibrationAxis& axis) const
{
    return workstationName + QChar(0x1f) + moduleKey + QChar(0x1f) + axis.name + QChar(0x1f) + QString::number(axis.axisNumber);
}

const WorkstationCalibration* WorkstationCalibrationPage::currentCalibration() const
{
    if (!m_selectedWorkstationName.isEmpty()) {
        for (const WorkstationCalibration& calibration : m_calibrations) {
            if (calibration.calibrationName == m_selectedWorkstationName) {
                return &calibration;
            }
        }
    }
    return m_calibrations.isEmpty() ? nullptr : &m_calibrations.first();
}

void WorkstationCalibrationPage::moveAxisByStep(const QString& workstationName, const QString& moduleKey, const QString& axisName, double direction)
{
    if (!m_calibrationInteractionEnabled) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Please wait for the current motion to finish"),
                                 QStringLiteral("\u8bf7\u7b49\u5f85\u5f53\u524d\u52a8\u4f5c\u5b8c\u6210")),
                          true);
        return;
    }

    if (!m_controllerService.isConnected()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Controller is not connected"),
                                 QStringLiteral("\u63a7\u5236\u5668\u672a\u8fde\u63a5")),
                          true);
        return;
    }

    QString errorMessage;
    const bool useHighSpeed = !containsString(m_lowJogSpeedWorkstations, workstationName);
    WorkstationCalibrationWorkflowService::JogMoveRequest request;
    if (!m_workflowService.prepareJogMove(m_calibrations,
                                          workstationName,
                                          moduleKey,
                                          axisName,
                                          direction,
                                          useHighSpeed,
                                          &request,
                                          &errorMessage)) {
        showStatusMessage(trText(m_languageCode, errorMessage, errorMessage), true);
        showFailureDialog(axisName, errorMessage);
        return;
    }
    setCalibrationInteractionEnabled(false);
    if (!m_executionService.moveAxisRelativeAndWait(request.axisNumber,
                                                    request.stepDistance,
                                                    request.speedValue,
                                                    request.targetPosition,
                                                    request.tolerance,
                                                    8000,
                                                    &errorMessage)) {
        setCalibrationInteractionEnabled(true);
        showStatusMessage(errorMessage, true);
        showFailureDialog(request.axisName, errorMessage);
        return;
    }
    setCalibrationInteractionEnabled(true);

    refreshAxisPositions();
    showStatusMessage(trText(m_languageCode,
                             QStringLiteral("Axis %1 moved by %2 %3 at %4")
                                 .arg(request.axisName)
                                 .arg(request.stepDistance, 0, 'f', 3)
                                 .arg(request.axisUnit)
                                 .arg(request.speedValue, 0, 'f', 3),
                             QStringLiteral("\u8f74 %1 \u5df2\u6309\u6b65\u957f\u79fb\u52a8 %2 %3\uff0c\u901f\u5ea6 %4")
                                 .arg(request.axisName)
                                 .arg(request.stepDistance, 0, 'f', 3)
                                 .arg(request.axisUnit)
                                 .arg(request.speedValue, 0, 'f', 3)),
                      false);
}

bool WorkstationCalibrationPage::moveAxisToTarget(const QString& workstationName, const QString& axisName, double targetPosition)
{
    if (!m_calibrationInteractionEnabled) {
        return false;
    }

    WorkstationCalibration* calibration = m_workflowService.findCalibration(&m_calibrations, workstationName);

    if (!calibration) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Workstation %1 was not found").arg(workstationName),
                                 QStringLiteral("\u672a\u627e\u5230\u5de5\u4f4d %1").arg(workstationName)),
                          true);
        return false;
    }

    const auto axisMatch = m_workflowService.findAxisByName(*calibration, axisName);
    const QString moduleKey = axisMatch.moduleKey;
    const CalibrationAxis* axis = axisMatch.axis;
    if (!axis) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Axis %1 was not found in move sequence").arg(axisName),
                                 QStringLiteral("\u79fb\u52a8\u987a\u5e8f\u4e2d\u672a\u627e\u5230\u8f74 %1").arg(axisName)),
                          true);
        return false;
    }

    if (!m_controllerService.isConnected()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Controller is not connected"),
                                 QStringLiteral("\u63a7\u5236\u5668\u672a\u8fde\u63a5")),
                          true);
        return false;
    }

    QString errorMessage;
    const bool useHighSpeed = !containsString(m_lowJogSpeedWorkstations, workstationName);
    const double speedValue = calibration
        ? m_workflowService.jogSpeedFor(*calibration, *axis, useHighSpeed)
        : axis->defaultSpeed;
    setCalibrationInteractionEnabled(false);
    if (!m_executionService.moveAxisToPositionAndWait(axis->axisNumber,
                                                      targetPosition,
                                                      speedValue,
                                                      0.02,
                                                      12000,
                                                      &errorMessage)) {
        setCalibrationInteractionEnabled(true);
        showStatusMessage(errorMessage, true);
        showFailureDialog(axis->name, errorMessage);
        return false;
    }
    setCalibrationInteractionEnabled(true);

    refreshAxisPositions();
    showStatusMessage(trText(m_languageCode,
                             QStringLiteral("Axis %1 moved to %2 %3 at %4")
                                 .arg(axis->name)
                                 .arg(targetPosition, 0, 'f', 3)
                                 .arg(axis->unit)
                                 .arg(speedValue, 0, 'f', 3),
                             QStringLiteral("\u8f74 %1 \u5df2\u79fb\u52a8\u5230 %2 %3\uff0c\u901f\u5ea6 %4")
                                 .arg(axis->name)
                                 .arg(targetPosition, 0, 'f', 3)
                                 .arg(axis->unit)
                                 .arg(speedValue, 0, 'f', 3)),
                      false);
    Q_UNUSED(moduleKey);
    return true;
}
bool WorkstationCalibrationPage::moveToStartPosition(const QString& workstationName)
{
    if (!m_calibrationInteractionEnabled) {
        return false;
    }

    WorkstationCalibration* calibration = m_workflowService.findCalibration(&m_calibrations, workstationName);

    if (!calibration) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Workstation %1 was not found").arg(workstationName),
                                 QStringLiteral("\u672a\u627e\u5230\u5de5\u4f4d %1").arg(workstationName)),
                          true);
        return false;
    }

    QString errorMessage;
    WorkstationCalibrationWorkflowService::StartPositionRequest request;
    if (!m_workflowService.prepareStartPositionRequest(m_calibrations, workstationName, &request, &errorMessage)) {
        showStatusMessage(trText(m_languageCode, errorMessage, errorMessage), true);
        return false;
    }
    setCalibrationInteractionEnabled(false);
    if (!m_executionService.executeCommandWithCompletion(request.command,
                                                         request.doneVariable,
                                                         request.doneBuffer,
                                                         request.doneValue,
                                                         &errorMessage)) {
        setCalibrationInteractionEnabled(true);
        errorMessage = trText(m_languageCode,
                              QStringLiteral("%1 failed: %2").arg(textForKey(QStringLiteral("start_position")), errorMessage),
                              QStringLiteral("%1 \u5931\u8d25\uff1a%2").arg(textForKey(QStringLiteral("start_position")), errorMessage));
        showStatusMessage(errorMessage, true);
        showFailureDialog(textForKey(QStringLiteral("start_position")), errorMessage);
        return false;
    }
    setCalibrationInteractionEnabled(true);

    m_selectedMoveStepByWorkstation.insert(workstationName, -1);
    m_reachedMoveStepsByWorkstation.remove(workstationName);
    addUniqueString(m_startPositionReachedWorkstations, workstationName);
    captureStartAxisPositions(*calibration);
    refreshAxisPositions();
    showStatusMessage(trText(m_languageCode,
                             QStringLiteral("Moved to start position"),
                             QStringLiteral("\u5df2\u79fb\u52a8\u5230\u8d77\u59cb\u4f4d")),
                      false);
    return true;
}
void WorkstationCalibrationPage::captureStartAxisPositions(const WorkstationCalibration& calibration)
{
    if (!m_controllerService.isConnected()) {
        return;
    }

    QMap<QString, double> positions;
    QString errorMessage;
    m_workflowService.captureAxisPositions(calibration, &positions, &errorMessage);
    m_startAxisPositionsByWorkstation.insert(calibration.calibrationName, positions);
}

bool WorkstationCalibrationPage::canExecuteMoveSequenceStep(const QString& workstationName, int targetIndex) const
{
    const bool startReached = containsString(m_startPositionReachedWorkstations, workstationName);
    const int currentIndex = m_selectedMoveStepByWorkstation.value(workstationName, -1);
    return (startReached && currentIndex < 0 && targetIndex == 0)
        || (startReached && currentIndex >= 0 && (targetIndex == currentIndex - 1 || targetIndex == currentIndex + 1));
}

bool WorkstationCalibrationPage::resolveReverseMoveTarget(const QString& workstationName,
                                                          int currentIndex,
                                                          QString* axisName,
                                                          double* reverseTarget,
                                                          QString* errorMessage) const
{
    if (!axisName || !reverseTarget) {
        return false;
    }

    const WorkstationCalibration* calibration = m_workflowService.findCalibration(m_calibrations, workstationName);
    if (!calibration) {
        if (errorMessage) {
            *errorMessage = trText(m_languageCode,
                                   QStringLiteral("Workstation %1 was not found").arg(workstationName),
                                   QStringLiteral("\u672a\u627e\u5230\u5de5\u4f4d %1").arg(workstationName));
        }
        return false;
    }

    if (currentIndex < 0 || currentIndex >= calibration->moveSequence.size()) {
        return false;
    }

    const CalibrationMoveStep& currentStep = calibration->moveSequence.at(currentIndex);
    *axisName = currentStep.axisName;

    for (int stepIndex = currentIndex - 1; stepIndex >= 0; --stepIndex) {
        const CalibrationMoveStep& previousStep = calibration->moveSequence.at(stepIndex);
        if (previousStep.axisName.compare(currentStep.axisName, Qt::CaseInsensitive) == 0) {
            *reverseTarget = previousStep.targetPosition;
            return true;
        }
    }

    const QMap<QString, double> startPositions = m_startAxisPositionsByWorkstation.value(workstationName);
    const QString axisKeyName = currentStep.axisName.toUpper();
    if (startPositions.contains(axisKeyName)) {
        *reverseTarget = startPositions.value(axisKeyName);
        return true;
    }

    if (errorMessage) {
        *errorMessage = trText(m_languageCode,
                               QStringLiteral("Please move to start position before reversing this step"),
                               QStringLiteral("\u8bf7\u5148\u56de\u5230\u8d77\u59cb\u4f4d\u540e\u518d\u53cd\u5411\u6267\u884c\u6b64\u6b65\u9aa4"));
    }
    return false;
}

void WorkstationCalibrationPage::updateMoveSequenceSelectionAfterMotion(const QString& workstationName,
                                                                        int currentIndex,
                                                                        int targetIndex)
{
    m_selectedMoveStepByWorkstation.insert(workstationName, targetIndex);

    QSet<int>& reached = m_reachedMoveStepsByWorkstation[workstationName];
    if (currentIndex >= 0 && targetIndex == currentIndex - 1) {
        for (int stepIndex = currentIndex; stepIndex > targetIndex; --stepIndex) {
            reached.remove(stepIndex);
        }
        return;
    }

    reached.insert(targetIndex);
}

void WorkstationCalibrationPage::handleMoveSequenceStepClick(const QString& workstationName,
                                                             const QString& axisName,
                                                             double targetPosition,
                                                             int stepIndex)
{
    const int currentIndex = m_selectedMoveStepByWorkstation.value(workstationName, -1);
    if (!canExecuteMoveSequenceStep(workstationName, stepIndex)) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Only the previous or next step can be executed"),
                                 QStringLiteral("\u4e00\u6b21\u53ea\u80fd\u6267\u884c\u4e0a\u4e00\u6b65\u6216\u4e0b\u4e00\u6b65")),
                          true);
        return;
    }

    QString moveAxisName = axisName;
    double moveTarget = targetPosition;
    if (currentIndex >= 0 && stepIndex == currentIndex - 1) {
        QString errorMessage;
        if (!resolveReverseMoveTarget(workstationName, currentIndex, &moveAxisName, &moveTarget, &errorMessage)) {
            if (!errorMessage.isEmpty()) {
                showStatusMessage(errorMessage, true);
            }
            return;
        }
    }

    if (!moveAxisToTarget(workstationName, moveAxisName, moveTarget)) {
        return;
    }

    updateMoveSequenceSelectionAfterMotion(workstationName, currentIndex, stepIndex);
    rebuildCalibrationView();
}

void WorkstationCalibrationPage::saveSelectedMoveSequencePosition(const QString& workstationName)
{
    WorkstationCalibration* calibration = m_workflowService.findCalibration(&m_calibrations, workstationName);

    if (!calibration) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Workstation %1 was not found").arg(workstationName),
                                 QStringLiteral("\u672a\u627e\u5230\u5de5\u4f4d %1").arg(workstationName)),
                          true);
        return;
    }

    const int selectedIndex = m_selectedMoveStepByWorkstation.value(workstationName, -1);
    if (selectedIndex < 0 || selectedIndex >= calibration->moveSequence.size()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Please select a move-sequence step first"),
                                 QStringLiteral("\u8bf7\u5148\u9009\u62e9\u4e00\u4e2a\u79fb\u52a8\u987a\u5e8f\u6b65\u9aa4")),
                          true);
        return;
    }

    if (!m_controllerService.isConnected()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Controller is not connected"),
                                 QStringLiteral("\u63a7\u5236\u5668\u672a\u8fde\u63a5")),
                          true);
        return;
    }

    QString errorMessage;
    WorkstationCalibrationWorkflowService::PendingStepUpdate update;
    if (!m_workflowService.readCurrentMoveSequencePosition(*calibration, selectedIndex, &update, &errorMessage)) {
        showStatusMessage(errorMessage, true);
        return;
    }

    calibration->moveSequence[update.stepIndex].targetPosition = update.position;
    if (!m_workflowService.saveCalibrations(configFilePath(), m_calibrations, &errorMessage)) {
        showStatusMessage(errorMessage, true);
        return;
    }

    rebuildCalibrationView();
    showStatusMessage(trText(m_languageCode,
                             QStringLiteral("Saved %1 target position as %2 %3")
                                 .arg(update.axisName)
                                 .arg(update.position, 0, 'f', 3)
                                 .arg(update.unit),
                             QStringLiteral("\u5df2\u5c06 %1 \u7684\u76ee\u6807\u4f4d\u7f6e\u4fdd\u5b58\u4e3a %2 %3")
                                 .arg(update.axisName)
                                 .arg(update.position, 0, 'f', 3)
                                 .arg(update.unit)),
                      false);
}

void WorkstationCalibrationPage::saveMoveSequencePositionsWithConfirmation(const QString& workstationName)
{
    WorkstationCalibration* calibration = m_workflowService.findCalibration(&m_calibrations, workstationName);

    if (!calibration) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Workstation %1 was not found").arg(workstationName),
                                 QStringLiteral("\u672a\u627e\u5230\u5de5\u4f4d %1").arg(workstationName)),
                          true);
        return;
    }

    const int selectedIndex = m_selectedMoveStepByWorkstation.value(workstationName, -1);
    if (selectedIndex < 0 || selectedIndex >= calibration->moveSequence.size()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Please select a move-sequence step first"),
                                 QStringLiteral("\u8bf7\u5148\u9009\u62e9\u4e00\u4e2a\u79fb\u52a8\u987a\u5e8f\u6b65\u9aa4")),
                          true);
        return;
    }

    if (!m_controllerService.isConnected()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("Controller is not connected"),
                                 QStringLiteral("\u63a7\u5236\u5668\u672a\u8fde\u63a5")),
                          true);
        return;
    }

    QVector<WorkstationCalibrationWorkflowService::PendingStepUpdate> pendingUpdates;
    QString errorMessage;
    if (!m_workflowService.collectPendingMoveSequenceUpdates(*calibration, selectedIndex, &pendingUpdates, &errorMessage)) {
        showStatusMessage(errorMessage, true);
        return;
    }

    if (pendingUpdates.isEmpty()) {
        showStatusMessage(trText(m_languageCode,
                                 QStringLiteral("No step positions are available to save"),
                                 QStringLiteral("\u6ca1\u6709\u53ef\u4fdd\u5b58\u7684\u6b65\u9aa4\u4f4d\u7f6e")),
                          true);
        return;
    }

    QStringList detailLines;
    for (const WorkstationCalibrationWorkflowService::PendingStepUpdate& update : pendingUpdates) {
        detailLines << trText(m_languageCode,
                              QStringLiteral("Step %1 | %2 | %3 %4")
                                  .arg(update.stepIndex + 1)
                                  .arg(update.axisName)
                                  .arg(update.position, 0, 'f', 3)
                                  .arg(update.unit),
                              QStringLiteral("\u6b65\u9aa4 %1 | %2 | %3 %4")
                                  .arg(update.stepIndex + 1)
                                  .arg(update.axisName)
                                  .arg(update.position, 0, 'f', 3)
                                  .arg(update.unit));
    }

    const QString confirmText = trText(
        m_languageCode,
        QStringLiteral("The following positions will be saved:\n\n%1\n\nContinue?")
            .arg(detailLines.join(QStringLiteral("\n"))),
        QStringLiteral("\u5c06\u4fdd\u5b58\u4ee5\u4e0b\u4f4d\u7f6e:\n\n%1\n\n\u662f\u5426\u7ee7\u7eed\uff1f")
            .arg(detailLines.join(QStringLiteral("\n"))));
    if (QMessageBox::question(this,
                              trText(m_languageCode, QStringLiteral("Confirm Save"), QStringLiteral("\u786e\u8ba4\u4fdd\u5b58")),
                              confirmText,
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) != QMessageBox::Yes) {
        return;
    }

    for (const WorkstationCalibrationWorkflowService::PendingStepUpdate& update : pendingUpdates) {
        calibration->moveSequence[update.stepIndex].targetPosition = update.position;
    }

    if (!m_workflowService.saveCalibrations(configFilePath(), m_calibrations, &errorMessage)) {
        showStatusMessage(errorMessage, true);
        return;
    }

    QTimer::singleShot(80, this, [this, detailLines]() {
        QMessageBox::information(
            this,
            trText(m_languageCode, QStringLiteral("Saved"), QStringLiteral("\u5df2\u4fdd\u5b58")),
            detailLines.join(QStringLiteral("\n")));
    });

    reloadConfig();
    showStatusMessage(trText(m_languageCode,
                             QStringLiteral("Saved %1 step positions").arg(pendingUpdates.size()),
                             QStringLiteral("\u5df2\u4fdd\u5b58 %1 \u4e2a\u6b65\u9aa4\u4f4d\u7f6e").arg(pendingUpdates.size())),
                      false);
}

void WorkstationCalibrationPage::bindAxisConfigEditors(const QString& workstationName,
                                                       const QVector<AxisSelection>& axisSelections,
                                                       QComboBox* axisCombo,
                                                       QDoubleSpinBox* speedSpin,
                                                       QDoubleSpinBox* stepSpin,
                                                       QLabel* unitLabel,
                                                       QLabel* moduleLabel)
{
    if (!axisCombo || !speedSpin || !stepSpin || !unitLabel) {
        return;
    }

    auto updateUiFromAxis = [this, workstationName, axisSelections, axisCombo, speedSpin, stepSpin, unitLabel, moduleLabel]() {
        const int index = axisCombo->currentIndex();
        if (index < 0 || index >= axisSelections.size()) {
            return;
        }

        const AxisSelection& selection = axisSelections.at(index);
        WorkstationCalibrationWorkflowService::AxisEditorState state;
        if (!m_workflowService.readAxisEditorState(m_calibrations,
                                                   workstationName,
                                                   selection.moduleKey,
                                                   selection.axisName,
                                                   &state)) {
            return;
        }

        {
            QSignalBlocker blocker(speedSpin);
            speedSpin->setValue(state.defaultSpeed);
        }
        {
            QSignalBlocker blocker(stepSpin);
            stepSpin->setValue(state.defaultStep);
        }
        unitLabel->setText(state.unit);

        if (moduleLabel && !state.moduleName.isEmpty()) {
            moduleLabel->setText(state.moduleName);
        }
    };

    connect(axisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [updateUiFromAxis](int) {
        updateUiFromAxis();
    });

    connect(speedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this, workstationName, axisSelections, axisCombo](double value) {
                const int index = axisCombo ? axisCombo->currentIndex() : -1;
                if (index < 0 || index >= axisSelections.size()) {
                    return;
                }
                const AxisSelection& selection = axisSelections.at(index);
                m_workflowService.updateAxisDefaultSpeed(&m_calibrations,
                                                         workstationName,
                                                         selection.moduleKey,
                                                         selection.axisName,
                                                         value);
            });

    connect(stepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this, workstationName, axisSelections, axisCombo](double value) {
                const int index = axisCombo ? axisCombo->currentIndex() : -1;
                if (index < 0 || index >= axisSelections.size()) {
                    return;
                }
                const AxisSelection& selection = axisSelections.at(index);
                m_workflowService.updateAxisDefaultStep(&m_calibrations,
                                                        workstationName,
                                                        selection.moduleKey,
                                                        selection.axisName,
                                                        value);
            });

    updateUiFromAxis();
}

QWidget* WorkstationCalibrationPage::createMoveSequenceCard(const WorkstationCalibration& calibration)
{
    m_moveSequenceUnifiedStepSpins.remove(calibration.calibrationName);
    m_startPositionButtons.remove(calibration.calibrationName);

    auto* groupBox = new QGroupBox(textForKey(QStringLiteral("move_sequence")), m_contentWidget);
    groupBox->setStyleSheet(QStringLiteral(
        "QGroupBox{font-size:16px;font-weight:700;color:#18324a;border:1px solid #d7dee5;"
        "border-radius:12px;margin-top:10px;background:#ffffff;}"
        "QGroupBox::title{subcontrol-origin:margin;left:14px;padding:0 6px;}"));

    auto* rootLayout = new QVBoxLayout(groupBox);
    rootLayout->setContentsMargins(14, 24, 14, 14);
    rootLayout->setSpacing(8);

    auto* toolbarRow = new QHBoxLayout();
    toolbarRow->setContentsMargins(0, 0, 0, 0);
    toolbarRow->setSpacing(8);

    if (m_workstationSwitchContainer) {
        m_workstationSwitchContainer->setParent(groupBox);
        m_workstationSwitchContainer->setVisible(m_workstationCombo && m_workstationCombo->count() > 1);
        toolbarRow->addWidget(m_workstationSwitchContainer, 0, Qt::AlignVCenter);
    }

    auto* speedModeLabel = createMiniLabel(textForKey(QStringLiteral("speed_mode")), groupBox);
    auto* lowModeButton = new QPushButton(textForKey(QStringLiteral("speed_low")), groupBox);
    auto* highModeButton = new QPushButton(textForKey(QStringLiteral("speed_high")), groupBox);
    lowModeButton->setCheckable(true);
    highModeButton->setCheckable(true);
    const bool useHighSpeed = !containsString(m_lowJogSpeedWorkstations, calibration.calibrationName);
    lowModeButton->setChecked(!useHighSpeed);
    highModeButton->setChecked(useHighSpeed);
    const QString toggleStyle = QStringLiteral(
        "QPushButton{background:#f5f8fb;border:1px solid #cdd8e3;border-radius:8px;padding:5px 10px;color:#214c74;}"
        "QPushButton:checked{background:#dbe9f5;border:1px solid #4f89bd;font-weight:700;}");
    lowModeButton->setStyleSheet(toggleStyle);
    highModeButton->setStyleSheet(toggleStyle);
    lowModeButton->setToolTip(trText(m_languageCode,
                                     QStringLiteral("Low speed: %1").arg(calibration.jogSpeedLow, 0, 'f', 3),
                                     QStringLiteral("\u4f4e\u901f: %1").arg(calibration.jogSpeedLow, 0, 'f', 3)));
    highModeButton->setToolTip(trText(m_languageCode,
                                      QStringLiteral("High speed: %1").arg(calibration.jogSpeedHigh, 0, 'f', 3),
                                      QStringLiteral("\u9ad8\u901f: %1").arg(calibration.jogSpeedHigh, 0, 'f', 3)));
    toolbarRow->addWidget(speedModeLabel);
    toolbarRow->addWidget(lowModeButton);
    toolbarRow->addWidget(highModeButton);

    connect(lowModeButton, &QPushButton::clicked, this, [this, workstationName = calibration.calibrationName, lowModeButton, highModeButton]() {
        addUniqueString(m_lowJogSpeedWorkstations, workstationName);
        lowModeButton->setChecked(true);
        highModeButton->setChecked(false);
    });
    connect(highModeButton, &QPushButton::clicked, this, [this, workstationName = calibration.calibrationName, lowModeButton, highModeButton]() {
        removeString(m_lowJogSpeedWorkstations, workstationName);
        lowModeButton->setChecked(false);
        highModeButton->setChecked(true);
    });
    auto* saveButton = new QPushButton(textForKey(QStringLiteral("save_current_position")), groupBox);
    saveButton->setStyleSheet(QStringLiteral(
        "QPushButton{background:#f4f8fc;border:1px solid #c9d6e2;border-radius:8px;"
        "padding:6px 12px;font-size:12px;font-weight:700;color:#214c74;}"
        "QPushButton:hover{background:#eaf2f9;border-color:#90abc4;}"
        "QPushButton:pressed{background:#dbe9f5;}"));
    connect(saveButton, &QPushButton::clicked, this, [this, workstationName = calibration.calibrationName]() {
        saveMoveSequencePositionsWithConfirmation(workstationName);
    });
    toolbarRow->addStretch();
    toolbarRow->addWidget(saveButton, 0, Qt::AlignVCenter);
    rootLayout->addLayout(toolbarRow);

    auto* scrollArea = new QScrollArea(groupBox);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setMinimumHeight(118);

    auto* container = new QWidget(scrollArea);
    auto* rowLayout = new QHBoxLayout(container);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);

    const int currentIndex = m_selectedMoveStepByWorkstation.value(calibration.calibrationName, -1);
    const QSet<int> reachedSteps = m_reachedMoveStepsByWorkstation.value(calibration.calibrationName);
    const bool startReached = containsString(m_startPositionReachedWorkstations, calibration.calibrationName);

    auto* startButton = new QPushButton(textForKey(QStringLiteral("start_position")), container);
    startButton->setFixedSize(92, 74);
    startButton->setStyleSheet((currentIndex < 0 && startReached)
        ? QStringLiteral(
            "QPushButton{background:#dbe9f5;border:2px solid #4f89bd;border-radius:10px;"
            "font-size:12px;font-weight:700;color:#163a5c;padding:5px;}")
        : startReached
        ? QStringLiteral(
            "QPushButton{background:#e8f6ea;border:1px solid #84c694;border-radius:10px;"
            "font-size:12px;font-weight:700;color:#23633a;padding:6px;}")
        : QStringLiteral(
            "QPushButton{background:#f8fafc;border:1px dashed #cbd7e3;border-radius:10px;"
            "font-size:12px;font-weight:700;color:#688099;padding:6px;}"));
    startButton->setEnabled(!calibration.startPositionCommand.trimmed().isEmpty());
    startButton->setToolTip(trText(m_languageCode,
                                   QStringLiteral("Click to execute the start-position command"),
                                   QStringLiteral("\u70b9\u51fb\u6267\u884c\u8d77\u59cb\u4f4d\u547d\u4ee4")));
    connect(startButton, &QPushButton::clicked, this, [this, workstationName = calibration.calibrationName]() {
        if (moveToStartPosition(workstationName)) {
            rebuildCalibrationView();
        }
    });
    m_startPositionButtons.insert(calibration.calibrationName, startButton);
    rowLayout->addWidget(startButton);

    if (!calibration.moveSequence.isEmpty()) {
        auto* startArrow = new QLabel(QString(QChar(0x2192)), container);
        startArrow->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#86a0b7;"));
        startArrow->setAlignment(Qt::AlignCenter);
        rowLayout->addWidget(startArrow);
    }

    for (int index = 0; index < calibration.moveSequence.size(); ++index) {
        const CalibrationMoveStep& step = calibration.moveSequence.at(index);
        const auto axisMatch = m_workflowService.findAxisByName(calibration, step.axisName);
        const CalibrationAxis* axis = axisMatch.axis;
        const QString unit = axis ? axis->unit : QStringLiteral("mm");
        const QString sensorVariable = step.arrivalSensorVariable.trimmed();
        const bool arrivalActive = false;

        auto* stepButton = new QPushButton(
            QStringLiteral("%1\n%2\n%3 %4")
                .arg(index + 1)
                .arg(step.axisName)
                .arg(step.targetPosition, 0, 'f', 3)
                .arg(unit),
            container);
        stepButton->setFixedSize(108, 74);
        const bool isCurrent = (index == currentIndex);
        const bool isReached = reachedSteps.contains(index);
        stepButton->setStyleSheet(arrivalActive && isCurrent
            ? QStringLiteral(
                "QPushButton{background:#d8effb;border:2px solid #2a8cc7;border-radius:10px;"
                "font-size:12px;font-weight:700;color:#0f4d74;padding:5px;}")
            : arrivalActive
            ? QStringLiteral(
                "QPushButton{background:#ecf8ee;border:2px solid #4aaf63;border-radius:10px;"
                "font-size:12px;font-weight:700;color:#21613a;padding:5px;}")
            : isCurrent
            ? QStringLiteral(
                "QPushButton{background:#dbe9f5;border:2px solid #4f89bd;border-radius:10px;"
                "font-size:12px;font-weight:700;color:#163a5c;padding:5px;}"
                "QPushButton:hover{background:#d4e4f2;}"
                "QPushButton:pressed{background:#c7dbec;}")
            : isReached
            ? QStringLiteral(
                "QPushButton{background:#e8f6ea;border:1px solid #84c694;border-radius:10px;"
                "font-size:12px;font-weight:700;color:#23633a;padding:6px;}"
                "QPushButton:hover{background:#def1e1;border-color:#66ae78;}"
                "QPushButton:pressed{background:#d0e8d5;}")
            : QStringLiteral(
                "QPushButton{background:#f5f8fb;border:1px solid #cdd8e3;border-radius:10px;"
                "font-size:12px;font-weight:700;color:#1f4b75;padding:6px;}"
                "QPushButton:hover{background:#eef5fb;border-color:#90abc4;}"
                "QPushButton:pressed{background:#ddeaf5;}"));
        stepButton->setProperty("arrivalSensorVariable", sensorVariable);
        stepButton->setProperty("stepIndex", index);
        stepButton->setProperty("workstationName", calibration.calibrationName);
        stepButton->setProperty("isCurrentMoveStep", isCurrent);
        stepButton->setProperty("isReachedMoveStep", isReached);
        stepButton->setProperty("isArrivalActive", arrivalActive);
        const bool canMoveFromStart = (startReached && currentIndex < 0 && index == 0);
        const bool canMoveAdjacent = (startReached && currentIndex >= 0 && (index == currentIndex - 1 || index == currentIndex + 1));
        stepButton->setEnabled(canMoveFromStart || canMoveAdjacent);
        stepButton->setToolTip(trText(m_languageCode,
                                      QStringLiteral("Move %1 to %2 %3. Only adjacent steps are allowed.").arg(step.axisName).arg(step.targetPosition, 0, 'f', 3).arg(unit),
                                      QStringLiteral("\u5c06 %1 \u79fb\u52a8\u5230 %2 %3\u3002\u4ec5\u5141\u8bb8\u6267\u884c\u76f8\u90bb\u6b65\u9aa4\u3002").arg(step.axisName).arg(step.targetPosition, 0, 'f', 3).arg(unit)));
        connect(stepButton, &QPushButton::clicked, this, [this, workstationName = calibration.calibrationName, axisName = step.axisName, targetPosition = step.targetPosition, index]() {
            handleMoveSequenceStepClick(workstationName, axisName, targetPosition, index);
        });
        m_moveStepButtons.insert(QStringLiteral("%1|%2").arg(calibration.calibrationName).arg(index), stepButton);
        rowLayout->addWidget(stepButton);

        if (index < calibration.moveSequence.size() - 1) {
            auto* arrowLabel = new QLabel(QString(QChar(0x2192)), container);
            arrowLabel->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#86a0b7;"));
            arrowLabel->setAlignment(Qt::AlignCenter);
            rowLayout->addWidget(arrowLabel);
        }
    }

    rowLayout->addStretch();
    scrollArea->setWidget(container);
    rootLayout->addWidget(scrollArea);

    return groupBox;
}

void WorkstationCalibrationPage::refreshAxisPositions()
{
    const WorkstationCalibration* calibration = currentCalibration();
    if (!calibration) {
        return;
    }

    for (const CalibrationModule& module : calibration->modules) {
        for (const CalibrationAxis& axis : module.axes) {
            QLineEdit* display = m_axisPositionDisplays.value(axisKey(calibration->calibrationName, module.key, axis), nullptr);
            if (!display) {
                continue;
            }

            if (!m_controllerService.isConnected()) {
                display->setText(QStringLiteral("--"));
                continue;
            }

            double position = 0.0;
            QString errorMessage;
            if (!m_controllerService.readAxisPosition(axis.axisNumber, &position, &errorMessage)) {
                display->setText(QStringLiteral("ERR"));
                display->setToolTip(QStringLiteral("%1 (#%2)\n%3").arg(axis.name).arg(axis.axisNumber).arg(errorMessage));
                continue;
            }

            display->setText(QString::number(position, 'f', 3));
            display->setToolTip(QStringLiteral("%1 (#%2)").arg(axis.name).arg(axis.axisNumber));
        }
    }

    for (int index = 0; index < calibration->moveSequence.size(); ++index) {
        QPushButton* stepButton = m_moveStepButtons.value(QStringLiteral("%1|%2").arg(calibration->calibrationName).arg(index), nullptr);
        if (!stepButton) {
            continue;
        }

        const QString sensorVariable = stepButton->property("arrivalSensorVariable").toString().trimmed();
        const bool isCurrent = stepButton->property("isCurrentMoveStep").toBool();
        const bool isReached = stepButton->property("isReachedMoveStep").toBool();
        bool arrivalActive = false;
        QString arrivalError;
        if (!sensorVariable.isEmpty() && m_controllerService.isConnected()) {
            int sensorValue = 0;
            if (m_controllerService.readIntegerVariable(sensorVariable, &sensorValue, &arrivalError)) {
                arrivalActive = sensorValue != 0;
            }
        }

        stepButton->setStyleSheet(arrivalActive && isCurrent
            ? QStringLiteral("QPushButton{background:#d8effb;border:2px solid #2a8cc7;border-radius:10px;font-size:12px;font-weight:700;color:#0f4d74;padding:5px;}")
            : arrivalActive
            ? QStringLiteral("QPushButton{background:#ecf8ee;border:2px solid #4aaf63;border-radius:10px;font-size:12px;font-weight:700;color:#21613a;padding:5px;}")
            : isCurrent
            ? QStringLiteral("QPushButton{background:#dbe9f5;border:2px solid #4f89bd;border-radius:10px;font-size:12px;font-weight:700;color:#163a5c;padding:5px;}QPushButton:hover{background:#d4e4f2;}QPushButton:pressed{background:#c7dbec;}")
            : isReached
            ? QStringLiteral("QPushButton{background:#e8f6ea;border:1px solid #84c694;border-radius:10px;font-size:12px;font-weight:700;color:#23633a;padding:6px;}QPushButton:hover{background:#def1e1;border-color:#66ae78;}QPushButton:pressed{background:#d0e8d5;}")
            : QStringLiteral("QPushButton{background:#f5f8fb;border:1px solid #cdd8e3;border-radius:10px;font-size:12px;font-weight:700;color:#1f4b75;padding:6px;}QPushButton:hover{background:#eef5fb;border-color:#90abc4;}QPushButton:pressed{background:#ddeaf5;}"));

        if (!sensorVariable.isEmpty()) {
            stepButton->setToolTip(trText(
                m_languageCode,
                QStringLiteral("Arrival sensor: %1").arg(sensorVariable),
                QStringLiteral("\u5230\u4f4d\u4f20\u611f\u5668: %1").arg(sensorVariable)));
        }
    }
}

QWidget* WorkstationCalibrationPage::createModuleWidget(const CalibrationModule& module, bool includeAxisConfig)
{
    const WorkstationCalibration* calibration = currentCalibration();
    auto* groupBox = new QGroupBox(module.name, m_contentWidget);
    groupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    groupBox->setStyleSheet(QStringLiteral(
        "QGroupBox{font-size:16px;font-weight:700;color:#18324a;border:1px solid #d7dee5;"
        "border-radius:12px;margin-top:10px;background:#ffffff;}"
        "QGroupBox::title{subcontrol-origin:margin;left:14px;padding:0 6px;}"));

    auto* rootLayout = new QVBoxLayout(groupBox);
    rootLayout->setContentsMargins(14, 24, 14, 14);
    rootLayout->setSpacing(10);

    if (!module.description.isEmpty()) {
        auto* descLabel = new QLabel(module.description, groupBox);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#5f6b76;"));
        rootLayout->addWidget(descLabel);
    }

    if (module.axes.isEmpty()) {
        if (calibration && !module.operations.isEmpty()) {
            rootLayout->addWidget(createModuleMotionCard(*calibration, module), 0, Qt::AlignLeft);
        } else {
            auto* emptyLabel = new QLabel(textForKey(QStringLiteral("module_empty")), groupBox);
            emptyLabel->setStyleSheet(QStringLiteral("font-size:13px;color:#5f6b76;"));
            rootLayout->addWidget(emptyLabel);
        }
        return groupBox;
    }

    auto* contentRow = new QHBoxLayout();
    contentRow->setContentsMargins(0, 0, 0, 0);
    contentRow->setSpacing(12);
    if (calibration) {
        contentRow->addWidget(createModuleMotionCard(*calibration, module), 0);
    }
    contentRow->addStretch();
    rootLayout->addLayout(contentRow);

    return groupBox;
}

QWidget* WorkstationCalibrationPage::createCalibrationPositionBar(const WorkstationCalibration& calibration)
{
    auto* card = new QFrame(m_contentWidget);
    card->setStyleSheet(QStringLiteral(
        "QFrame{background:transparent;border:none;}"));
    card->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    auto* rootLayout = new QVBoxLayout(card);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* barLayout = new QHBoxLayout();
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(8);

    struct AxisWithModule
    {
        QString moduleKey;
        CalibrationAxis axis;
    };
    QVector<AxisWithModule> orderedAxes;
    for (const CalibrationModule& module : calibration.modules) {
        for (const CalibrationAxis& axis : module.axes) {
            orderedAxes.push_back({module.key, axis});
        }
    }

    auto axisRank = [](const QString& axisName) {
        const QString upper = axisName.trimmed().toUpper();
        if (upper.startsWith(QStringLiteral("X"))) return 0;
        if (upper.startsWith(QStringLiteral("Y"))) return 1;
        if (upper.startsWith(QStringLiteral("Z"))) return 2;
        if (upper.startsWith(QStringLiteral("T"))) return 3;
        return 9;
    };
    std::sort(orderedAxes.begin(), orderedAxes.end(), [&](const AxisWithModule& left, const AxisWithModule& right) {
        const int leftRank = axisRank(left.axis.name);
        const int rightRank = axisRank(right.axis.name);
        if (leftRank != rightRank) {
            return leftRank < rightRank;
        }

        QRegularExpression re(QStringLiteral("(\\d+)$"));
        const auto leftMatch = re.match(left.axis.name);
        const auto rightMatch = re.match(right.axis.name);
        const int leftIndex = leftMatch.hasMatch() ? leftMatch.captured(1).toInt() : 0;
        const int rightIndex = rightMatch.hasMatch() ? rightMatch.captured(1).toInt() : 0;
        if (leftIndex != rightIndex) {
            return leftIndex < rightIndex;
        }
        return left.axis.name < right.axis.name;
    });

    for (const AxisWithModule& item : orderedAxes) {
        auto* axisFrame = new QFrame(card);
        axisFrame->setStyleSheet(QStringLiteral(
            "QFrame{background:#f6f9fc;border:1px solid #d6e1eb;border-radius:8px;}"));
        auto* axisLayout = new QHBoxLayout(axisFrame);
        axisLayout->setContentsMargins(8, 4, 8, 4);
        axisLayout->setSpacing(6);

        auto* axisLabel = createMiniLabel(item.axis.name, axisFrame);
        axisLabel->setStyleSheet(QStringLiteral("font-size:11px;font-weight:700;color:#1f4b75;"));
        axisLayout->addWidget(axisLabel);
        QLineEdit* display = createPositionDisplay(item.axis, axisFrame);
        display->setFixedWidth(72);
        display->setStyleSheet(QStringLiteral(
            "QLineEdit{background:transparent;border:none;padding:0 2px;"
            "font-size:12px;font-weight:700;color:#17344f;}"));
        m_axisPositionDisplays.insert(axisKey(calibration.calibrationName, item.moduleKey, item.axis), display);
        axisLayout->addWidget(display);
        barLayout->addWidget(axisFrame);
    }

    rootLayout->addLayout(barLayout);
    return card;
}

QWidget* WorkstationCalibrationPage::createModuleMotionCard(const WorkstationCalibration& calibration, const CalibrationModule& module)
{
    auto* card = createCard(m_contentWidget, 280);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    if (!module.axes.isEmpty()) {
        layout->addWidget(createSectionTitle(QStringLiteral("Jog"), card));
    }

    auto addActionButtons = [this, card, &module](QBoxLayout* targetLayout) {
        if (!targetLayout || module.operations.isEmpty()) {
            return;
        }

        auto addActionButton = [this, card, targetLayout](const QString& labelText,
                                                          const QIcon& icon,
                                                          const CalibrationCommandAction& action,
                                                          const QString& styleSheet) {
            auto* button = new QPushButton(card);
            button->setIcon(icon);
            button->setText(labelText);
            button->setIconSize(QSize(26, 26));
            button->setFixedSize(116, 44);
            button->setToolTip(labelText);
            button->setCursor(Qt::PointingHandCursor);
            button->setLayoutDirection(Qt::LeftToRight);
            button->setStyleSheet(styleSheet);
            targetLayout->addWidget(button);

            connect(button, &QPushButton::clicked, this, [this, action, labelText]() {
                QString errorMessage;
                setCalibrationInteractionEnabled(false);
                if (!m_executionService.executeCommandWithCompletion(action.command,
                                                                     action.doneVariable,
                                                                     action.doneBuffer,
                                                                     action.doneValue,
                                                                     &errorMessage)) {
                    setCalibrationInteractionEnabled(true);
                    errorMessage = trText(m_languageCode,
                                          QStringLiteral("%1 failed: %2").arg(labelText, errorMessage),
                                          QStringLiteral("%1 \u5931\u8d25\uff1a%2").arg(labelText, errorMessage));
                    showStatusMessage(errorMessage, true);
                    showFailureDialog(labelText, errorMessage);
                    return;
                }
                setCalibrationInteractionEnabled(true);
                showStatusMessage(trText(m_languageCode,
                                         QStringLiteral("%1 completed").arg(labelText),
                                         QStringLiteral("%1 \u5df2\u5b8c\u6210").arg(labelText)),
                                  false);
            });
        };

        if (moduleHasOperation(module, QStringLiteral("grip"))) {
            const CalibrationCommandAction action = moduleOperationAction(module, QStringLiteral("grip"));
            addActionButton(trText(m_languageCode, QStringLiteral("Grip"), QStringLiteral("\u5939\u6301")),
                            createClampActionIcon(true),
                            action,
                            QStringLiteral("QPushButton{background:#eef8f2;border:1px solid #b9dcc5;border-radius:12px;"
                                           "padding:0 12px;text-align:left;font-size:12px;font-weight:700;color:#1f6f43;}"
                                           "QPushButton:hover{background:#e4f2ea;border-color:#8dc1a2;}"
                                           "QPushButton:pressed{background:#d7ebdf;}"));
        }
        if (moduleHasOperation(module, QStringLiteral("release"))) {
            const CalibrationCommandAction action = moduleOperationAction(module, QStringLiteral("release"));
            addActionButton(trText(m_languageCode, QStringLiteral("Release"), QStringLiteral("\u5f20\u5f00")),
                            createClampActionIcon(false),
                            action,
                            QStringLiteral("QPushButton{background:#eef4fb;border:1px solid #bdd2e7;border-radius:12px;"
                                           "padding:0 12px;text-align:left;font-size:12px;font-weight:700;color:#2b5f8a;}"
                                           "QPushButton:hover{background:#e4edf8;border-color:#94b5d6;}"
                                           "QPushButton:pressed{background:#d6e4f4;}"));
        }
    };

    if (module.axes.isEmpty()) {
        auto* actionRow = new QHBoxLayout();
        actionRow->setContentsMargins(0, 0, 0, 0);
        actionRow->setSpacing(0);
        actionRow->addSpacing(204);

        auto* actionColumn = new QVBoxLayout();
        actionColumn->setContentsMargins(8, 0, 0, 0);
        actionColumn->setSpacing(10);
        addActionButtons(actionColumn);
        actionColumn->addStretch();
        actionRow->addLayout(actionColumn);
        actionRow->addStretch();
        layout->addLayout(actionRow);
        return card;
    }

    auto* motionRow = new QHBoxLayout();
    motionRow->setContentsMargins(0, 0, 0, 0);
    motionRow->setSpacing(10);

    bool hasX = false;
    bool hasY = false;
    for (const CalibrationAxis& axis : module.axes) {
        const QString type = axisTypeOf(axis);
        hasX = hasX || type == QStringLiteral("X");
        hasY = hasY || type == QStringLiteral("Y");
    }

    if (hasX || hasY) {
        auto* xyFrame = new QFrame(card);
        xyFrame->setStyleSheet(QStringLiteral(
            "QFrame{background:transparent;border:none;}"));
        auto* xyLayout = new QGridLayout(xyFrame);
        xyLayout->setContentsMargins(0, 0, 0, 0);
        xyLayout->setHorizontalSpacing(4);
        xyLayout->setVerticalSpacing(4);

        auto* center = new QLabel(QStringLiteral("XY"), xyFrame);
        center->setAlignment(Qt::AlignCenter);
        center->setFixedSize(34, 34);
        center->setStyleSheet(QStringLiteral(
            "QLabel{background:#eef5fb;border:none;border-radius:17px;"
            "font-size:11px;font-weight:700;color:#1f4b75;}"));

        auto* upButton = createRoundJogButton(arrowUp(), xyFrame);
        auto* leftButton = createRoundJogButton(arrowLeft(), xyFrame);
        auto* rightButton = createRoundJogButton(arrowRight(), xyFrame);
        auto* downButton = createRoundJogButton(arrowDown(), xyFrame);

        xyLayout->addWidget(upButton, 0, 1, Qt::AlignCenter);
        xyLayout->addWidget(leftButton, 1, 0, Qt::AlignCenter);
        xyLayout->addWidget(center, 1, 1, Qt::AlignCenter);
        xyLayout->addWidget(rightButton, 1, 2, Qt::AlignCenter);
        xyLayout->addWidget(downButton, 2, 1, Qt::AlignCenter);
        motionRow->addWidget(xyFrame, 0, Qt::AlignTop);

        QString xAxisName;
        QString yAxisName;
        for (const CalibrationAxis& axis : module.axes) {
            const QString type = axisTypeOf(axis);
            if (xAxisName.isEmpty() && type == QStringLiteral("X")) {
                xAxisName = axis.name;
            } else if (yAxisName.isEmpty() && type == QStringLiteral("Y")) {
                yAxisName = axis.name;
            }
        }

        if (!yAxisName.isEmpty()) {
            connect(upButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, yAxisName]() {
                moveAxisByStep(calibrationName, moduleKey, yAxisName, 1.0);
            });
            connect(downButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, yAxisName]() {
                moveAxisByStep(calibrationName, moduleKey, yAxisName, -1.0);
            });
        } else {
            upButton->setEnabled(false);
            downButton->setEnabled(false);
        }

        if (!xAxisName.isEmpty()) {
            connect(leftButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, xAxisName]() {
                moveAxisByStep(calibrationName, moduleKey, xAxisName, -1.0);
            });
            connect(rightButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, xAxisName]() {
                moveAxisByStep(calibrationName, moduleKey, xAxisName, 1.0);
            });
        } else {
            leftButton->setEnabled(false);
            rightButton->setEnabled(false);
        }
    }

    for (const CalibrationAxis& axis : module.axes) {
        const QString type = axisTypeOf(axis);
        if (type == QStringLiteral("X") || type == QStringLiteral("Y")) {
            continue;
        }

        auto* axisFrame = new QFrame(card);
        axisFrame->setStyleSheet(QStringLiteral(
            "QFrame{background:transparent;border:none;}"));
        auto* axisLayout = new QVBoxLayout(axisFrame);
        axisLayout->setContentsMargins(0, 0, 0, 0);
        axisLayout->setSpacing(6);

        auto* axisLabel = new QLabel(axis.name, axisFrame);
        axisLabel->setAlignment(Qt::AlignCenter);
        axisLabel->setStyleSheet(QStringLiteral("font-size:11px;font-weight:700;color:#1f4b75;"));
        axisLayout->addWidget(axisLabel);

        if (type == QStringLiteral("Z")) {
            auto* upButton = createRoundJogButton(arrowUp(), axisFrame);
            auto* downButton = createRoundJogButton(arrowDown(), axisFrame);
            axisLayout->addWidget(upButton, 0, Qt::AlignHCenter);
            axisLayout->addWidget(downButton, 0, Qt::AlignHCenter);
            connect(upButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, axisName = axis.name]() {
                moveAxisByStep(calibrationName, moduleKey, axisName, 1.0);
            });
            connect(downButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, axisName = axis.name]() {
                moveAxisByStep(calibrationName, moduleKey, axisName, -1.0);
            });
        } else if (type == QStringLiteral("T")) {
            auto* rotateRow = new QHBoxLayout();
            rotateRow->setContentsMargins(0, 0, 0, 0);
            rotateRow->setSpacing(6);
            auto* ccwButton = createRoundJogButton(rotateCcw(), axisFrame);
            auto* cwButton = createRoundJogButton(rotateCw(), axisFrame);
            rotateRow->addWidget(ccwButton);
            rotateRow->addWidget(cwButton);
            axisLayout->addLayout(rotateRow);
            connect(ccwButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, axisName = axis.name]() {
                moveAxisByStep(calibrationName, moduleKey, axisName, -1.0);
            });
            connect(cwButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, axisName = axis.name]() {
                moveAxisByStep(calibrationName, moduleKey, axisName, 1.0);
            });
        } else {
            auto* moveRow = new QHBoxLayout();
            moveRow->setContentsMargins(0, 0, 0, 0);
            moveRow->setSpacing(6);
            auto* negativeButton = createRoundJogButton(arrowLeft(), axisFrame);
            auto* positiveButton = createRoundJogButton(arrowRight(), axisFrame);
            moveRow->addWidget(negativeButton);
            moveRow->addWidget(positiveButton);
            axisLayout->addLayout(moveRow);
            connect(negativeButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, axisName = axis.name]() {
                moveAxisByStep(calibrationName, moduleKey, axisName, -1.0);
            });
            connect(positiveButton, &QPushButton::clicked, this, [this, calibrationName = calibration.calibrationName, moduleKey = module.key, axisName = axis.name]() {
                moveAxisByStep(calibrationName, moduleKey, axisName, 1.0);
            });
        }

        motionRow->addWidget(axisFrame, 0, Qt::AlignTop);
    }

    if (!module.operations.isEmpty()) {
        auto* actionColumn = new QVBoxLayout();
        actionColumn->setContentsMargins(8, 0, 0, 0);
        actionColumn->setSpacing(10);
        addActionButtons(actionColumn);
        actionColumn->addStretch();
        motionRow->addLayout(actionColumn);
    }

    motionRow->addStretch();
    if (isForkModule(module) || isEdgeChuckModule(module)) {
        motionRow->addWidget(createModulePositionCard(module), 0, Qt::AlignTop | Qt::AlignRight);
    }
    layout->addLayout(motionRow);
    return card;
}

QWidget* WorkstationCalibrationPage::createCombinedAxisConfigCard(const WorkstationCalibration& calibration, const CalibrationModule& leftModule, const CalibrationModule& rightModule)
{
    auto* card = createCard(m_contentWidget);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(createSectionTitle(trText(m_languageCode, QStringLiteral("Combined Axis Config"), QStringLiteral("\u8f74\u914d\u7f6e")), card));

    auto* selectorRow = new QHBoxLayout();
    selectorRow->setContentsMargins(0, 0, 0, 0);
    selectorRow->setSpacing(8);
    selectorRow->addWidget(createMiniLabel(textForKey(QStringLiteral("axis")), card));

    auto* axisCombo = new QComboBox(card);
    axisCombo->setMinimumWidth(260);
    selectorRow->addWidget(axisCombo, 0);
    selectorRow->addStretch();
    layout->addLayout(selectorRow);

    auto* valueRow = new QHBoxLayout();
    valueRow->setContentsMargins(0, 0, 0, 0);
    valueRow->setSpacing(8);

    auto* speedLabel = createMiniLabel(textForKey(QStringLiteral("speed")), card);
    auto* speedSpin = createAxisSpinBox(leftModule.axes.first().defaultSpeed, 3, 0.001);
    auto* stepLabel = createMiniLabel(textForKey(QStringLiteral("step")), card);
    auto* stepSpin = createAxisSpinBox(leftModule.axes.first().defaultStep, 3, 0.001);
    auto* unitLabel = createMiniLabel(leftModule.axes.first().unit, card);
    auto* moduleLabel = createMiniLabel(leftModule.name, card);
    moduleLabel->setStyleSheet(QStringLiteral("font-size:11px;font-weight:700;color:#1f4b75;"));

    valueRow->addWidget(moduleLabel);
    valueRow->addSpacing(8);
    valueRow->addWidget(speedLabel);
    valueRow->addWidget(speedSpin);
    valueRow->addWidget(stepLabel);
    valueRow->addWidget(stepSpin);
    valueRow->addWidget(unitLabel);
    valueRow->addStretch();
    layout->addLayout(valueRow);

    QVector<AxisSelection> selections;
    selections.reserve(leftModule.axes.size() + rightModule.axes.size());
    for (const CalibrationAxis& axis : leftModule.axes) {
        selections.push_back({calibration.calibrationName, leftModule.key, axis.name});
    }
    for (const CalibrationAxis& axis : rightModule.axes) {
        selections.push_back({calibration.calibrationName, rightModule.key, axis.name});
    }

    for (const AxisSelection& selection : selections) {
        WorkstationCalibrationWorkflowService::AxisEditorState state;
        if (m_workflowService.readAxisEditorState(m_calibrations,
                                                  selection.workstationName,
                                                  selection.moduleKey,
                                                  selection.axisName,
                                                  &state)) {
            axisCombo->addItem(QStringLiteral("%1 - %2 (#%3)")
                                   .arg(state.moduleName)
                                   .arg(selection.axisName)
                                   .arg(state.axisNumber));
        }
    }

    bindAxisConfigEditors(calibration.calibrationName, selections, axisCombo, speedSpin, stepSpin, unitLabel, moduleLabel);

    return card;
}

QWidget* WorkstationCalibrationPage::createModuleAxisConfigCard(const WorkstationCalibration& calibration, const CalibrationModule& module)
{
    auto* card = createCard(m_contentWidget, 350);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(createSectionTitle(textForKey(QStringLiteral("config_editor")), card));

    auto* selectorRow = new QHBoxLayout();
    selectorRow->setContentsMargins(0, 0, 0, 0);
    selectorRow->setSpacing(6);

    auto* axisLabel = createMiniLabel(textForKey(QStringLiteral("axis")), card);
    auto* axisCombo = new QComboBox(card);
    axisCombo->setMinimumWidth(170);
    selectorRow->addWidget(axisLabel);
    selectorRow->addWidget(axisCombo, 1);
    layout->addLayout(selectorRow);

    auto* valueRow = new QHBoxLayout();
    valueRow->setContentsMargins(0, 0, 0, 0);
    valueRow->setSpacing(6);

    auto* speedLabel = createMiniLabel(textForKey(QStringLiteral("speed")), card);
    auto* speedSpin = createAxisSpinBox(module.axes.first().defaultSpeed, 3, 0.001);
    auto* stepLabel = createMiniLabel(textForKey(QStringLiteral("step")), card);
    auto* stepSpin = createAxisSpinBox(module.axes.first().defaultStep, 3, 0.001);
    auto* unitLabel = createMiniLabel(module.axes.first().unit, card);

    valueRow->addWidget(speedLabel);
    valueRow->addWidget(speedSpin);
    valueRow->addWidget(stepLabel);
    valueRow->addWidget(stepSpin);
    valueRow->addWidget(unitLabel);
    valueRow->addStretch();
    layout->addLayout(valueRow);

    QVector<AxisSelection> selections;
    selections.reserve(module.axes.size());
    for (const CalibrationAxis& axis : module.axes) {
        axisCombo->addItem(QStringLiteral("%1 (#%2)").arg(axis.name).arg(axis.axisNumber));
        selections.push_back({calibration.calibrationName, module.key, axis.name});
    }

    bindAxisConfigEditors(calibration.calibrationName, selections, axisCombo, speedSpin, stepSpin, unitLabel);

    auto* hintLabel = createMiniLabel(textForKey(QStringLiteral("config_path")), card);
    hintLabel->setText(textForKey(QStringLiteral("config_editor")));
    layout->addWidget(hintLabel);

    return card;
}

QWidget* WorkstationCalibrationPage::createModulePositionCard(const CalibrationModule& module)
{
    auto* card = createCard(m_contentWidget, 380);
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);
    layout->addWidget(createSectionTitle(QStringLiteral("Axis Position"), card));

    for (const CalibrationAxis& axis : module.axes) {
        auto* row = new QHBoxLayout();
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(6);

        auto* axisLabel = new QLabel(QStringLiteral("%1 (#%2)").arg(axis.name).arg(axis.axisNumber), card);
        axisLabel->setFixedWidth(84);
        axisLabel->setStyleSheet(QStringLiteral("font-size:12px;font-weight:700;color:#1f3447;"));

        row->addWidget(axisLabel);
        QLineEdit* display = createPositionDisplay(axis, card);
        m_axisPositionDisplays.insert(axisKey(currentCalibration() ? currentCalibration()->calibrationName : QString(),
                                              module.key,
                                              axis),
                                      display);
        row->addWidget(display);
        row->addWidget(createMiniLabel(axis.unit, card));
        row->addStretch();
        layout->addLayout(row);
    }

    return card;
}

QWidget* WorkstationCalibrationPage::createWorkstationSwitchWidget(QWidget* parent)
{
    auto* container = new QWidget(parent);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* label = createMiniLabel(textForKey(QStringLiteral("workstation")), container);
    auto* combo = new QComboBox(container);
    combo->setMinimumWidth(220);
    for (const WorkstationCalibration& calibration : m_calibrations) {
        combo->addItem(calibration.calibrationName);
    }

    QString selectedName = m_selectedWorkstationName;
    if (selectedName.isEmpty() && !m_calibrations.isEmpty()) {
        selectedName = m_calibrations.first().calibrationName;
    }
    const int index = combo->findText(selectedName);
    combo->setCurrentIndex(index >= 0 ? index : 0);

    layout->addWidget(label);
    layout->addWidget(combo);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &WorkstationCalibrationPage::handleWorkstationChanged);
    return container;
}

QPushButton* WorkstationCalibrationPage::createSettingsLaunchButton(QWidget* parent) const
{
    auto* button = new QPushButton(textForKey(QStringLiteral("settings")), parent);
    button->setProperty("settingsLauncher", true);
    button->setStyleSheet(UiHelpers::secondaryButtonStyle());
    connect(button, &QPushButton::clicked, this, &WorkstationCalibrationPage::openConfigEditorDialog);
    return button;
}

QDoubleSpinBox* WorkstationCalibrationPage::createAxisSpinBox(double value, int decimals, double minimum, const QString& suffix) const
{
    auto* spinBox = new QDoubleSpinBox();
    spinBox->setDecimals(decimals);
    spinBox->setRange(minimum, 100000.0);
    spinBox->setValue(value);
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setAlignment(Qt::AlignCenter);
    spinBox->setFixedWidth(78);
    spinBox->setStyleSheet(QStringLiteral(
        "QDoubleSpinBox{background:transparent;border:none;"
        "padding:0 2px;font-size:11px;color:#1f3447;}"));
    if (!suffix.isEmpty()) {
        spinBox->setSuffix(suffix);
    }
    return spinBox;
}

QPushButton* WorkstationCalibrationPage::createJogButton(const QString& text) const
{
    return createRoundJogButton(text, nullptr);
}



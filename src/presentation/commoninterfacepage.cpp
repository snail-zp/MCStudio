#include "presentation/commoninterfacepage.h"
#include <QAbstractItemView>
#include "presentation/uihelpers.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
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
#include <QMap>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextEdit>
#include <QThread>
#include <QTimer>
#include <QShowEvent>
#include <QVBoxLayout>

#include <cmath>
#include <functional>

namespace
{
QTableWidgetItem* makeItem(const QString& text)
{
    return new QTableWidgetItem(text);
}

class MaterialTransferTopologyWidget : public QWidget
{
public:
    explicit MaterialTransferTopologyWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(360);
        auto* grid = new QGridLayout(this);
        grid->setContentsMargins(24, 24, 24, 24);
        grid->setHorizontalSpacing(18);
        grid->setVerticalSpacing(18);
        grid->setColumnStretch(0, 1);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(2, 1);
        grid->setRowStretch(0, 1);
        grid->setRowStretch(1, 1);
        grid->setRowStretch(2, 1);
        m_gridLayout = grid;
    }

    void setStationClickHandler(std::function<void(const CommonInterfaceMaterialTransferStation&)> clickHandler)
    {
        m_clickHandler = std::move(clickHandler);
    }

    void setRobotClickHandler(std::function<void()> robotClickHandler)
    {
        m_robotClickHandler = std::move(robotClickHandler);
    }

    void setConfig(const CommonInterfaceMaterialTransferConfig& config)
    {
        m_config = config;

        if (!m_robotButton) {
            m_robotButton = new QPushButton(this);
            m_robotButton->setCursor(Qt::PointingHandCursor);
            m_robotButton->setMinimumSize(96, 96);
            connect(m_robotButton, &QPushButton::clicked, this, [this]() {
                if (m_robotClickHandler) {
                    m_robotClickHandler();
                }
            });
            m_gridLayout->addWidget(m_robotButton, 1, 1, Qt::AlignCenter);
        }
        const bool materialOnRobot = !m_config.currentMaterialLocation.isEmpty()
            && m_config.currentMaterialLocation.compare(m_config.robotName, Qt::CaseInsensitive) == 0;
        m_robotButton->setText(materialOnRobot
                                   ? QStringLiteral("%1\n[MATERIAL]").arg(m_config.robotName.isEmpty() ? QStringLiteral("Robot") : m_config.robotName)
                                   : (m_config.robotName.isEmpty() ? QStringLiteral("Robot") : m_config.robotName));
        m_robotButton->setToolTip(materialOnRobot
                                      ? QStringLiteral("Material: HERE")
                                      : QStringLiteral("Click to move material onto robot"));
        m_robotButton->setStyleSheet(QStringLiteral(
            "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 %1,stop:1 %2);"
            "color:white;border:2px solid %3;border-radius:40px;font-size:16px;font-weight:700;}")
                                      .arg(materialOnRobot ? QStringLiteral("#d18f00") : QStringLiteral("#1c577f"))
                                      .arg(materialOnRobot ? QStringLiteral("#f3b61f") : QStringLiteral("#2d7cae"))
                                      .arg(materialOnRobot ? QStringLiteral("#ffd86a") : QStringLiteral("#93bfdb")));

        while (m_stationButtons.size() < m_config.stations.size()) {
            auto* button = new QPushButton(this);
            button->setCursor(Qt::PointingHandCursor);
            button->setMinimumSize(118, 52);
            connect(button, &QPushButton::clicked, this, [this, button]() {
                if (!m_clickHandler) {
                    return;
                }
                bool ok = false;
                const int index = button->property("stationIndex").toInt(&ok);
                if (!ok || index < 0 || index >= m_config.stations.size()) {
                    return;
                }
                const CommonInterfaceMaterialTransferStation station = m_config.stations.at(index);
                m_clickHandler(station);
            });
            m_stationButtons.push_back(button);
        }

        static const QVector<QPair<int, int>> kPositions = {
            {0, 1},
            {1, 2},
            {2, 1},
            {1, 0},
            {0, 0},
            {0, 2},
            {2, 2},
            {2, 0}
        };

        for (int index = 0; index < m_stationButtons.size(); ++index) {
            auto* button = m_stationButtons.at(index);
            if (index >= m_config.stations.size()) {
                button->hide();
                continue;
            }

            const CommonInterfaceMaterialTransferStation& station = m_config.stations.at(index);
            const bool materialHere = !m_config.currentMaterialLocation.isEmpty()
                && m_config.currentMaterialLocation.compare(station.stationName, Qt::CaseInsensitive) == 0;
            button->setText(materialHere ? QStringLiteral("%1\n[MATERIAL]").arg(station.stationName) : station.stationName);
            button->setStyleSheet(QStringLiteral(
                "QPushButton{background:%1;border:2px solid %2;border-radius:18px;"
                "padding:8px 10px;color:#18324a;font-size:13px;font-weight:700;}"
                "QPushButton:hover{border-color:#4d7fa6;background:#eef6fc;}")
                                      .arg(materialHere ? QStringLiteral("#ffe49a") : (station.color.isEmpty() ? QStringLiteral("#d9e8f5") : station.color))
                                      .arg(materialHere ? QStringLiteral("#d18f00") : QStringLiteral("#9db8cf")));
            QStringList tooltipLines;
            tooltipLines << station.stationName;
            if (materialHere) {
                tooltipLines << QStringLiteral("Material: HERE");
            }
            if (!station.pickupCommand.isEmpty()) {
                tooltipLines << QStringLiteral("Pick: %1").arg(station.pickupCommand);
            }
            if (!station.placeCommand.isEmpty()) {
                tooltipLines << QStringLiteral("Place: %1").arg(station.placeCommand);
            }
            button->setToolTip(tooltipLines.join(QLatin1Char('\n')));
            button->setProperty("stationIndex", index);
            const auto position = kPositions.at(index % kPositions.size());
            m_gridLayout->addWidget(button, position.first, position.second, Qt::AlignCenter);
            button->show();
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        if (!m_robotButton) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QPoint robotCenter = m_robotButton->geometry().center();
        const bool materialOnRobot = !m_config.currentMaterialLocation.isEmpty()
            && m_config.currentMaterialLocation.compare(m_config.robotName, Qt::CaseInsensitive) == 0;

        for (int index = 0; index < m_stationButtons.size() && index < m_config.stations.size(); ++index) {
            auto* button = m_stationButtons.at(index);
            if (!button || !button->isVisible()) {
                continue;
            }

            const CommonInterfaceMaterialTransferStation& station = m_config.stations.at(index);
            const bool materialHere = !m_config.currentMaterialLocation.isEmpty()
                && m_config.currentMaterialLocation.compare(station.stationName, Qt::CaseInsensitive) == 0;

            QPen pen(materialHere || materialOnRobot ? QColor(QStringLiteral("#d18f00"))
                                                     : QColor(QStringLiteral("#8fb3cf")));
            pen.setWidth(materialHere || materialOnRobot ? 4 : 3);
            pen.setCapStyle(Qt::RoundCap);
            painter.setPen(pen);
            painter.drawLine(robotCenter, button->geometry().center());
        }
    }

    CommonInterfaceMaterialTransferConfig m_config;
    std::function<void(const CommonInterfaceMaterialTransferStation&)> m_clickHandler;
    std::function<void()> m_robotClickHandler;
    QGridLayout* m_gridLayout = nullptr;
    QPushButton* m_robotButton = nullptr;
    QVector<QPushButton*> m_stationButtons;
};
}

CommonInterfacePage::CommonInterfacePage(ControllerService& controllerService, QWidget* parent)
    : LoggedPageWidget(FileLogger::Category::CommonInterface, parent)
    , m_controllerService(controllerService)
    , m_executionService(controllerService)
    , m_configFilePath(configFilePath())
{
    buildUi();
    updateWatcher();
    retranslateUi();
}

void CommonInterfacePage::setLanguage(const QString& languageCode)
{
    const QString normalized = languageCode.trimmed().compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
                                   ? QStringLiteral("en-US")
                                   : QStringLiteral("zh-CN");
    if (m_languageCode == normalized) {
        return;
    }
    m_languageCode = normalized;
    retranslateUi();
    if (m_initialConfigLoaded) {
        rebuildCommandGroups();
    }
}

void CommonInterfacePage::reloadConfig()
{
    if (m_actionInProgress) {
        m_reloadDeferred = true;
        return;
    }

    QString errorMessage;
    m_config = m_service.loadConfig(m_configFilePath, &errorMessage);
    if (m_configEditorDialog) {
        populateEditorTable();
    }
    rebuildCommandGroups();

    if (!errorMessage.isEmpty() && m_config.commands.isEmpty() && m_config.materialTransfer.stations.isEmpty()) {
        showStatusMessage(errorMessage, true);
        logMessage(errorMessage);
        return;
    }

    const QString message = tr("%1 loaded, total commands: %2, transfer stations: %3")
                                .arg(m_configFilePath)
                                .arg(m_config.commands.size())
                                .arg(m_config.materialTransfer.stations.size());
    showStatusMessage(message, false);
    logMessage(message);
}

void CommonInterfacePage::showEvent(QShowEvent* event)
{
    LoggedPageWidget::showEvent(event);
    if (m_initialConfigLoaded) {
        return;
    }

    m_initialConfigLoaded = true;
    QTimer::singleShot(0, this, &CommonInterfacePage::reloadConfig);
}

void CommonInterfacePage::flushDeferredReload()
{
    if (!m_reloadDeferred || m_actionInProgress) {
        return;
    }

    m_reloadDeferred = false;
    QTimer::singleShot(0, this, &CommonInterfacePage::reloadConfig);
}

void CommonInterfacePage::openConfigEditorDialog()
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

void CommonInterfacePage::browseConfigFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          textForKey(QStringLiteral("switch_config")),
                                                          QFileInfo(m_configFilePath).absolutePath(),
                                                          QStringLiteral("JSON Files (*.json);;All Files (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    m_configFilePath = filePath;
    updateWatcher();
    reloadConfig();
    if (m_editorConfigFileEdit) {
        m_editorConfigFileEdit->setText(m_configFilePath);
    }
}

void CommonInterfacePage::addCommandRow()
{
    if (m_editorTabWidget && m_editorTabWidget->currentIndex() == 1) {
        if (!m_editorTransferTable) {
            return;
        }
        const int row = m_editorTransferTable->rowCount();
        m_editorTransferTable->insertRow(row);
        m_editorTransferTable->setItem(row, 0, makeItem(QStringLiteral("Station-%1").arg(row + 1)));
        m_editorTransferTable->setItem(row, 1, makeItem(QStringLiteral("start 23,RobotArm_Pick_Station%1").arg(row + 1)));
        m_editorTransferTable->setItem(row, 2, makeItem(QStringLiteral("")));
        m_editorTransferTable->setItem(row, 3, makeItem(QStringLiteral("1")));
        m_editorTransferTable->setItem(row, 4, makeItem(QStringLiteral("start 23,RobotArm_Place_Station%1").arg(row + 1)));
        m_editorTransferTable->setItem(row, 5, makeItem(QStringLiteral("")));
        m_editorTransferTable->setItem(row, 6, makeItem(QStringLiteral("1")));
        m_editorTransferTable->setItem(row, 7, makeItem(QStringLiteral("#d9e8f5")));
        m_editorTransferTable->setCurrentCell(row, 0);
        return;
    }

    if (!m_editorTable) {
        return;
    }

    const int row = m_editorTable->rowCount();
    m_editorTable->insertRow(row);
    m_editorTable->setItem(row, 0, makeItem(QStringLiteral("General")));
    m_editorTable->setItem(row, 1, makeItem(QStringLiteral("New Command")));
    m_editorTable->setItem(row, 2, makeItem(QStringLiteral("")));
    m_editorTable->setItem(row, 3, makeItem(QStringLiteral("")));
    m_editorTable->setItem(row, 4, makeItem(QStringLiteral("-1")));
    m_editorTable->setItem(row, 5, makeItem(QStringLiteral("1")));
    m_editorTable->setCurrentCell(row, 0);
}

void CommonInterfacePage::removeSelectedCommandRow()
{
    if (m_editorTabWidget && m_editorTabWidget->currentIndex() == 1) {
        if (!m_editorTransferTable) {
            return;
        }
        const int row = m_editorTransferTable->currentRow();
        if (row >= 0) {
            m_editorTransferTable->removeRow(row);
        }
        return;
    }

    if (!m_editorTable) {
        return;
    }
    const int row = m_editorTable->currentRow();
    if (row >= 0) {
        m_editorTable->removeRow(row);
    }
}

void CommonInterfacePage::saveConfig()
{
    QString errorMessage;
    const CommonInterfaceCommandList commands = collectCommandsFromEditor(&errorMessage);
    if (!errorMessage.isEmpty()) {
        showStatusMessage(errorMessage, true);
        return;
    }

    const CommonInterfaceMaterialTransferConfig transferConfig = collectTransferConfigFromEditor(&errorMessage);
    if (!errorMessage.isEmpty()) {
        showStatusMessage(errorMessage, true);
        return;
    }

    m_config.commands = commands;
    m_config.materialTransfer = transferConfig;
    m_ignoreConfigWatcherChanges = true;
    if (!m_service.saveConfig(m_configFilePath, m_config, &errorMessage)) {
        m_ignoreConfigWatcherChanges = false;
        showStatusMessage(errorMessage, true);
        return;
    }

    rebuildCommandGroups();
    showStatusMessage(textForKey(QStringLiteral("save_success")), false);
    logMessage(textForKey(QStringLiteral("save_success")));
}

void CommonInterfacePage::handleConfigFileChanged(const QString& path)
{
    if (m_actionInProgress) {
        Q_UNUSED(path);
        return;
    }

    if (m_ignoreConfigWatcherChanges) {
        m_ignoreConfigWatcherChanges = false;
        if (m_configWatcher && QFileInfo::exists(path) && !m_configWatcher->files().contains(path)) {
            m_configWatcher->addPath(path);
        }
        return;
    }

    if (!QFileInfo::exists(path)) {
        logMessage(QStringLiteral("Common interface config changed via replace: %1").arg(path));
        QTimer::singleShot(150, this, [this]() {
            updateWatcher();
            reloadConfig();
        });
        return;
    }

    if (m_configWatcher && !m_configWatcher->files().contains(path)) {
        m_configWatcher->addPath(path);
    }

    QTimer::singleShot(150, this, &CommonInterfacePage::reloadConfig);
}

void CommonInterfacePage::handleConfigDirectoryChanged(const QString& path)
{
    Q_UNUSED(path);
    if (m_actionInProgress) {
        return;
    }
    if (m_ignoreConfigWatcherChanges) {
        m_ignoreConfigWatcherChanges = false;
        updateWatcher();
        return;
    }
    QTimer::singleShot(150, this, [this]() {
        updateWatcher();
        reloadConfig();
    });
}

void CommonInterfacePage::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(14);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_settingsButton = new QPushButton(this);
    m_settingsButton->setStyleSheet(UiHelpers::secondaryButtonStyle());
    connect(m_settingsButton, &QPushButton::clicked, this, &CommonInterfacePage::openConfigEditorDialog);

    auto* topRowLayout = new QHBoxLayout();
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(10);
    topRowLayout->addWidget(m_statusLabel, 1);
    topRowLayout->addWidget(m_settingsButton, 0, Qt::AlignTop);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_contentWidget = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(14);
    m_scrollArea->setWidget(m_contentWidget);

    m_runtimeLogLabel = new QLabel(this);
    m_runtimeLogLabel->hide();
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(160);
    m_logView->setStyleSheet(UiHelpers::logViewStyle());
    bindFeedbackWidgets(m_statusLabel, m_logView);

    rootLayout->addLayout(topRowLayout);
    rootLayout->addWidget(m_scrollArea, 1);
    rootLayout->addWidget(m_logView);

    // Keep common-interface actions local. External file watching used to rebuild the
    // whole page after every persisted transfer state change, which made button
    // clicks feel like a full refresh and could destabilize startup on Windows.
    m_configWatcher = nullptr;
}

void CommonInterfacePage::buildConfigEditorDialog()
{
    if (m_configEditorDialog) {
        return;
    }

    m_configEditorDialog = new QDialog(this);
    m_configEditorDialog->setModal(false);

    auto* rootLayout = new QVBoxLayout(m_configEditorDialog);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    m_editorTitleLabel = new QLabel(m_configEditorDialog);
    m_editorConfigFileLabel = new QLabel(m_configEditorDialog);
    m_editorConfigFileEdit = new QLineEdit(m_configEditorDialog);
    m_editorConfigFileEdit->setReadOnly(true);
    m_editorBrowseButton = new QPushButton(m_configEditorDialog);
    m_editorAddButton = new QPushButton(m_configEditorDialog);
    m_editorRemoveButton = new QPushButton(m_configEditorDialog);
    m_editorSaveButton = new QPushButton(m_configEditorDialog);
    m_editorRobotNameLabel = new QLabel(m_configEditorDialog);
    m_editorRobotNameEdit = new QLineEdit(m_configEditorDialog);
    m_editorMaterialLocationLabel = new QLabel(m_configEditorDialog);
    m_editorMaterialLocationEdit = new QLineEdit(m_configEditorDialog);
    m_editorTable = new QTableWidget(m_configEditorDialog);
    m_editorTable->setColumnCount(6);
    m_editorTable->horizontalHeader()->setStretchLastSection(true);
    m_editorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_editorTable->verticalHeader()->setVisible(false);
    m_editorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_editorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorTransferTable = new QTableWidget(m_configEditorDialog);
    m_editorTransferTable->setColumnCount(8);
    m_editorTransferTable->horizontalHeader()->setStretchLastSection(true);
    m_editorTransferTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_editorTransferTable->verticalHeader()->setVisible(false);
    m_editorTransferTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_editorTransferTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorTabWidget = new QTabWidget(m_configEditorDialog);

    auto* fileLayout = new QHBoxLayout();
    fileLayout->addWidget(m_editorConfigFileLabel);
    fileLayout->addWidget(m_editorConfigFileEdit, 1);
    fileLayout->addWidget(m_editorBrowseButton);

    auto* transferMetaLayout = new QHBoxLayout();
    transferMetaLayout->addWidget(m_editorRobotNameLabel);
    transferMetaLayout->addWidget(m_editorRobotNameEdit, 1);
    transferMetaLayout->addSpacing(12);
    transferMetaLayout->addWidget(m_editorMaterialLocationLabel);
    transferMetaLayout->addWidget(m_editorMaterialLocationEdit, 1);

    auto* actionLayout = new QHBoxLayout();
    actionLayout->addWidget(m_editorAddButton);
    actionLayout->addWidget(m_editorRemoveButton);
    actionLayout->addWidget(m_editorSaveButton);
    actionLayout->addStretch();

    auto* commandTab = new QWidget(m_configEditorDialog);
    auto* commandTabLayout = new QVBoxLayout(commandTab);
    commandTabLayout->setContentsMargins(0, 0, 0, 0);
    commandTabLayout->addWidget(m_editorTable);

    auto* transferTab = new QWidget(m_configEditorDialog);
    auto* transferTabLayout = new QVBoxLayout(transferTab);
    transferTabLayout->setContentsMargins(0, 0, 0, 0);
    transferTabLayout->setSpacing(10);
    transferTabLayout->addLayout(transferMetaLayout);
    transferTabLayout->addWidget(m_editorTransferTable);

    m_editorTabWidget->addTab(commandTab, QString());
    m_editorTabWidget->addTab(transferTab, QString());

    rootLayout->addWidget(m_editorTitleLabel);
    rootLayout->addLayout(fileLayout);
    rootLayout->addLayout(actionLayout);
    rootLayout->addWidget(m_editorTabWidget, 1);

    connect(m_editorBrowseButton, &QPushButton::clicked, this, &CommonInterfacePage::browseConfigFile);
    connect(m_editorAddButton, &QPushButton::clicked, this, &CommonInterfacePage::addCommandRow);
    connect(m_editorRemoveButton, &QPushButton::clicked, this, &CommonInterfacePage::removeSelectedCommandRow);
    connect(m_editorSaveButton, &QPushButton::clicked, this, &CommonInterfacePage::saveConfig);
}

void CommonInterfacePage::retranslateUi()
{
    if (m_settingsButton) m_settingsButton->setText(textForKey(QStringLiteral("settings")));
    if (m_runtimeLogLabel) m_runtimeLogLabel->setText(textForKey(QStringLiteral("runtime_log")));
    if (m_editorTitleLabel) m_editorTitleLabel->setText(textForKey(QStringLiteral("editor_title")));
    if (m_editorConfigFileLabel) m_editorConfigFileLabel->setText(textForKey(QStringLiteral("config_file")));
    if (m_editorBrowseButton) m_editorBrowseButton->setText(textForKey(QStringLiteral("browse")));
    if (m_editorAddButton) m_editorAddButton->setText(textForKey(QStringLiteral("add")));
    if (m_editorRemoveButton) m_editorRemoveButton->setText(textForKey(QStringLiteral("remove")));
    if (m_editorSaveButton) m_editorSaveButton->setText(textForKey(QStringLiteral("save")));
    if (m_editorRobotNameLabel) m_editorRobotNameLabel->setText(textForKey(QStringLiteral("robot_name")));
    if (m_editorMaterialLocationLabel) m_editorMaterialLocationLabel->setText(textForKey(QStringLiteral("material_location")));
    if (m_editorConfigFileEdit) m_editorConfigFileEdit->setText(m_configFilePath);
    if (m_configEditorDialog) {
        m_configEditorDialog->setWindowTitle(textForKey(QStringLiteral("editor_title")));
        m_configEditorDialog->resize(1180, 720);
    }
    if (m_editorTabWidget) {
        m_editorTabWidget->setTabText(0, textForKey(QStringLiteral("tab_commands")));
        m_editorTabWidget->setTabText(1, textForKey(QStringLiteral("tab_transfer")));
    }
    if (m_editorTable) {
        m_editorTable->setHorizontalHeaderLabels({
            textForKey(QStringLiteral("column_category")),
            textForKey(QStringLiteral("column_name")),
            textForKey(QStringLiteral("column_command")),
            textForKey(QStringLiteral("column_done_variable")),
            textForKey(QStringLiteral("column_done_buffer")),
            textForKey(QStringLiteral("column_done_value"))
        });
    }
    if (m_editorTransferTable) {
        m_editorTransferTable->setHorizontalHeaderLabels({
            textForKey(QStringLiteral("column_station_name")),
            textForKey(QStringLiteral("column_pickup_command")),
            textForKey(QStringLiteral("column_pickup_done_variable")),
            textForKey(QStringLiteral("column_pickup_done_value")),
            textForKey(QStringLiteral("column_place_command")),
            textForKey(QStringLiteral("column_place_done_variable")),
            textForKey(QStringLiteral("column_place_done_value")),
            textForKey(QStringLiteral("column_color"))
        });
    }
    if (m_statusLabel && m_statusLabel->text().isEmpty()) {
        showStatusMessage(textForKey(QStringLiteral("ready")), false);
    }
}

void CommonInterfacePage::rebuildCommandGroups()
{
    if (!m_contentLayout) {
        return;
    }

    while (m_contentLayout->count() > 0) {
        QLayoutItem* item = m_contentLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    QMap<QString, CommonInterfaceCommandList> groups;
    for (const CommonInterfaceCommand& command : m_config.commands) {
        groups[command.category].push_back(command);
    }

    QGroupBox* transferGroup = nullptr;
    if (!m_config.materialTransfer.robotName.isEmpty() && !m_config.materialTransfer.stations.isEmpty()) {
        transferGroup = new QGroupBox(textForKey(QStringLiteral("material_transfer_title")), m_contentWidget);
        transferGroup->setStyleSheet(UiHelpers::groupBoxStyle());
        auto* transferLayout = new QVBoxLayout(transferGroup);
        transferLayout->setContentsMargins(14, 18, 14, 14);
        transferLayout->setSpacing(10);

        auto* descriptionLabel = new QLabel(textForKey(QStringLiteral("material_transfer_description")), transferGroup);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setStyleSheet(QStringLiteral("font-size:13px;color:#5f6b76;"));
        transferLayout->addWidget(descriptionLabel);

        auto* materialLocationLabel = new QLabel(
            QStringLiteral("%1: %2")
                .arg(textForKey(QStringLiteral("material_location")))
                .arg(m_config.materialTransfer.currentMaterialLocation.isEmpty()
                         ? textForKey(QStringLiteral("unknown"))
                         : m_config.materialTransfer.currentMaterialLocation),
            transferGroup);
        materialLocationLabel->setStyleSheet(QStringLiteral(
            "font-size:13px;font-weight:700;color:#8a5a00;background:#fff4d6;border:1px solid #f0d18e;border-radius:8px;padding:8px 10px;"));
        transferLayout->addWidget(materialLocationLabel);

        auto* topologyWidget = new MaterialTransferTopologyWidget(transferGroup);
        QPointer<MaterialTransferTopologyWidget> safeTopologyWidget(topologyWidget);
        QPointer<QLabel> safeMaterialLocationLabel(materialLocationLabel);
        topologyWidget->setStationClickHandler(
            [this, safeTopologyWidget, safeMaterialLocationLabel](const CommonInterfaceMaterialTransferStation& station) {
                const CommonInterfaceMaterialTransferStation stationCopy = station;
                handleTransferStationExecution(stationCopy);
                if (safeMaterialLocationLabel) {
                    safeMaterialLocationLabel->setText(
                        QStringLiteral("%1: %2")
                            .arg(textForKey(QStringLiteral("material_location")))
                            .arg(m_config.materialTransfer.currentMaterialLocation.isEmpty()
                                     ? textForKey(QStringLiteral("unknown"))
                                     : m_config.materialTransfer.currentMaterialLocation));
                }
                if (safeTopologyWidget) {
                    safeTopologyWidget->setConfig(m_config.materialTransfer);
                }
            });
        topologyWidget->setRobotClickHandler(
            [this, safeTopologyWidget, safeMaterialLocationLabel]() {
                handleTransferToRobotExecution();
                if (safeMaterialLocationLabel) {
                    safeMaterialLocationLabel->setText(
                        QStringLiteral("%1: %2")
                            .arg(textForKey(QStringLiteral("material_location")))
                            .arg(m_config.materialTransfer.currentMaterialLocation.isEmpty()
                                     ? textForKey(QStringLiteral("unknown"))
                                     : m_config.materialTransfer.currentMaterialLocation));
                }
                if (safeTopologyWidget) {
                    safeTopologyWidget->setConfig(m_config.materialTransfer);
                }
            });
        topologyWidget->setConfig(m_config.materialTransfer);
        transferLayout->addWidget(topologyWidget);
    }

    QGroupBox* commandGroup = nullptr;
    if (!groups.isEmpty()) {
        commandGroup = new QGroupBox(textForKey(QStringLiteral("tab_commands")), m_contentWidget);
        commandGroup->setStyleSheet(UiHelpers::groupBoxStyle());
        auto* commandGroupLayout = new QVBoxLayout(commandGroup);
        commandGroupLayout->setContentsMargins(14, 18, 14, 14);
        commandGroupLayout->setSpacing(12);

        for (auto it = groups.cbegin(); it != groups.cend(); ++it) {
            auto* groupBox = new QGroupBox(it.key(), commandGroup);
            groupBox->setStyleSheet(QStringLiteral("QGroupBox{font-size:15px;font-weight:700;color:#2b4a63;}"));
            auto* layout = new QGridLayout(groupBox);
            layout->setContentsMargins(14, 18, 14, 14);
            layout->setHorizontalSpacing(10);
            layout->setVerticalSpacing(10);

            for (int index = 0; index < it.value().size(); ++index) {
                const CommonInterfaceCommand command = it.value().at(index);
                auto* button = new QPushButton(command.name, groupBox);
                button->setMinimumHeight(48);
                button->setCursor(Qt::PointingHandCursor);
                button->setStyleSheet(QStringLiteral(
                    "QPushButton{padding:12px 16px;border-radius:12px;"
                    "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #ffffff,stop:1 #eef6fd);"
                    "border:1px solid #c9d8e6;color:#16324a;text-align:left;font-size:13px;font-weight:600;}"
                    "QPushButton:hover{background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #ffffff,stop:1 #dcecff);"
                    "border-color:#6e9bc5;color:#0f2d45;}"
                    "QPushButton:pressed{background:#d7e8f7;border-color:#4d81b1;}"
                    "QPushButton:disabled{background:#f5f7fa;border-color:#d8e0e8;color:#8a97a4;}"));
                const QString tooltip = command.doneVariable.isEmpty()
                                            ? command.commandText
                                            : QStringLiteral("%1\n%2 @ buffer %3 = %4")
                                                  .arg(command.commandText, command.doneVariable)
                                                  .arg(command.doneBuffer)
                                                  .arg(command.doneValue);
                button->setToolTip(tooltip);
                connect(button, &QPushButton::clicked, this, [this, command]() {
                    handleCommandExecution(command);
                });
                layout->addWidget(button, index / 3, index % 3);
            }

            commandGroupLayout->addWidget(groupBox);
        }
        commandGroupLayout->addStretch();
    }

    if (transferGroup || commandGroup) {
        auto* topRowWidget = new QWidget(m_contentWidget);
        auto* topRowLayout = new QHBoxLayout(topRowWidget);
        topRowLayout->setContentsMargins(0, 0, 0, 0);
        topRowLayout->setSpacing(14);
        if (transferGroup) {
            topRowLayout->addWidget(transferGroup, 5);
        }
        if (commandGroup) {
            topRowLayout->addWidget(commandGroup, 4);
        }
        m_contentLayout->addWidget(topRowWidget);
    }

    if (groups.isEmpty() && m_config.materialTransfer.stations.isEmpty()) {
        auto* emptyLabel = new QLabel(textForKey(QStringLiteral("no_commands")), m_contentWidget);
        emptyLabel->setWordWrap(true);
        emptyLabel->setStyleSheet(QStringLiteral("font-size:14px;color:#5f6b76;"));
        m_contentLayout->addWidget(emptyLabel);
    }

    m_contentLayout->addStretch();
}

void CommonInterfacePage::populateEditorTable()
{
    if (!m_editorTable) {
        return;
    }

    QSignalBlocker blocker(m_editorTable);
    m_editorTable->setRowCount(m_config.commands.size());
    for (int row = 0; row < m_config.commands.size(); ++row) {
        const CommonInterfaceCommand& command = m_config.commands.at(row);
        m_editorTable->setItem(row, 0, makeItem(command.category));
        m_editorTable->setItem(row, 1, makeItem(command.name));
        m_editorTable->setItem(row, 2, makeItem(command.commandText));
        m_editorTable->setItem(row, 3, makeItem(command.doneVariable));
        m_editorTable->setItem(row, 4, makeItem(QString::number(command.doneBuffer)));
        m_editorTable->setItem(row, 5, makeItem(QString::number(command.doneValue)));
    }
    if (m_editorRobotNameEdit) {
        m_editorRobotNameEdit->setText(m_config.materialTransfer.robotName);
    }
    if (m_editorMaterialLocationEdit) {
        m_editorMaterialLocationEdit->setText(m_config.materialTransfer.currentMaterialLocation);
    }
    if (m_editorTransferTable) {
        m_editorTransferTable->setRowCount(m_config.materialTransfer.stations.size());
        for (int row = 0; row < m_config.materialTransfer.stations.size(); ++row) {
            const CommonInterfaceMaterialTransferStation& station = m_config.materialTransfer.stations.at(row);
            m_editorTransferTable->setItem(row, 0, makeItem(station.stationName));
            m_editorTransferTable->setItem(row, 1, makeItem(station.pickupCommand));
            m_editorTransferTable->setItem(row, 2, makeItem(station.pickupDoneVariable));
            m_editorTransferTable->setItem(row, 3, makeItem(QString::number(station.pickupDoneValue)));
            m_editorTransferTable->setItem(row, 4, makeItem(station.placeCommand));
            m_editorTransferTable->setItem(row, 5, makeItem(station.placeDoneVariable));
            m_editorTransferTable->setItem(row, 6, makeItem(QString::number(station.placeDoneValue)));
            m_editorTransferTable->setItem(row, 7, makeItem(station.color));
        }
    }
    if (m_editorConfigFileEdit) {
        m_editorConfigFileEdit->setText(m_configFilePath);
    }
}

void CommonInterfacePage::updateWatcher()
{
    // QFileSystemWatcher is intentionally disabled for this page. Reloads are now
    // explicit (open page, choose config file, save config), while command buttons
    // only refresh the widgets whose state actually changed.
}

void CommonInterfacePage::showStatusMessage(const QString& text, bool isError)
{
    setStatusMessage(text, isError, false);
}

void CommonInterfacePage::logMessage(const QString& message)
{
    appendLogMessage(message);
}

bool CommonInterfacePage::persistTransferConfigState()
{
    QString saveError;
    m_ignoreConfigWatcherChanges = true;
    if (!m_service.saveConfig(m_configFilePath, m_config, &saveError)) {
        m_ignoreConfigWatcherChanges = false;
        logMessage(saveError);
        return false;
    }
    if (m_editorMaterialLocationEdit) {
        m_editorMaterialLocationEdit->setText(m_config.materialTransfer.currentMaterialLocation);
    }
    updateWatcher();
    return true;
}

void CommonInterfacePage::handleCommandExecution(const CommonInterfaceCommand& command)
{
    if (m_actionInProgress) {
        return;
    }
    m_actionInProgress = true;
    logMessage(QStringLiteral("Execute command start: %1").arg(command.name));

    QString errorMessage;
    if (!m_executionService.executeCommand(command, &errorMessage)) {
        if (errorMessage == QStringLiteral("Controller is not connected")) {
            errorMessage = textForKey(QStringLiteral("not_connected"));
        }
        showStatusMessage(errorMessage, true);
        logMessage(errorMessage);
    } else {
        const QString message = textForKey(QStringLiteral("execute_success")).arg(command.name);
        showStatusMessage(message, false);
        logMessage(QStringLiteral("Execute command done: %1").arg(command.name));
        logMessage(message);
    }

    m_actionInProgress = false;
    m_reloadDeferred = false;
}

bool CommonInterfacePage::executeTransferStation(const CommonInterfaceMaterialTransferStation& station, QString* errorMessage)
{
    if (m_workflowService.isMaterialAtStation(m_config.materialTransfer, station)) {
        return true;
    }

    if (!m_workflowService.isMaterialOnRobot(m_config.materialTransfer)) {
        if (const CommonInterfaceMaterialTransferStation* currentStation =
                m_workflowService.currentSourceStation(m_config.materialTransfer)) {
            if (!m_executionService.executeTransferAction(currentStation->pickupCommand,
                                                          currentStation->pickupDoneVariable,
                                                          currentStation->pickupDoneValue,
                                                          errorMessage)) {
                return false;
            }
            m_workflowService.moveMaterialToRobot(&m_config.materialTransfer);
        }
    }

    if (!m_executionService.executeTransferAction(station.placeCommand,
                                                  station.placeDoneVariable,
                                                  station.placeDoneValue,
                                                  errorMessage)) {
        return false;
    }

    m_workflowService.moveMaterialToStation(&m_config.materialTransfer, station.stationName);
    persistTransferConfigState();
    return true;
}

bool CommonInterfacePage::executeTransferToRobot(QString* errorMessage)
{
    QString workflowError;
    if (!m_workflowService.canTransferToRobot(m_config.materialTransfer, &workflowError)) {
        if (errorMessage) {
            *errorMessage = textForKey(QStringLiteral("transfer_command_missing")).arg(workflowError);
        }
        return false;
    }

    if (const CommonInterfaceMaterialTransferStation* station =
            m_workflowService.currentSourceStation(m_config.materialTransfer)) {
        if (!m_executionService.executeTransferAction(station->pickupCommand,
                                                      station->pickupDoneVariable,
                                                      station->pickupDoneValue,
                                                      errorMessage)) {
            return false;
        }
    }

    m_workflowService.moveMaterialToRobot(&m_config.materialTransfer);
    persistTransferConfigState();
    return true;
}

void CommonInterfacePage::handleTransferStationExecution(const CommonInterfaceMaterialTransferStation& station)
{
    if (m_actionInProgress) {
        return;
    }
    m_actionInProgress = true;
    logMessage(QStringLiteral("Transfer station click: %1").arg(station.stationName));

    QString errorMessage;
    if (!executeTransferStation(station, &errorMessage)) {
        if (errorMessage == QStringLiteral("Controller is not connected")) {
            errorMessage = textForKey(QStringLiteral("not_connected"));
        }
        showStatusMessage(errorMessage, true);
        logMessage(errorMessage);
    } else {
        const QString message = textForKey(QStringLiteral("transfer_success")).arg(station.stationName);
        showStatusMessage(message, false);
        logMessage(message);
    }

    m_actionInProgress = false;
    m_reloadDeferred = false;
}

void CommonInterfacePage::handleTransferToRobotExecution()
{
    if (m_actionInProgress) {
        return;
    }
    m_actionInProgress = true;
    logMessage(QStringLiteral("Topology center robot clicked"));

    QString errorMessage;
    if (!executeTransferToRobot(&errorMessage)) {
        if (errorMessage == QStringLiteral("Controller is not connected")) {
            errorMessage = textForKey(QStringLiteral("not_connected"));
        }
        showStatusMessage(errorMessage, true);
        logMessage(errorMessage);
    } else {
        const QString robotName = m_config.materialTransfer.robotName.isEmpty()
            ? QStringLiteral("Robot")
            : m_config.materialTransfer.robotName;
        const QString message = textForKey(QStringLiteral("transfer_success")).arg(robotName);
        showStatusMessage(message, false);
        logMessage(message);
    }

    m_actionInProgress = false;
    m_reloadDeferred = false;
}

CommonInterfaceCommandList CommonInterfacePage::collectCommandsFromEditor(QString* errorMessage) const
{
    CommonInterfaceCommandList commands;
    if (!m_editorTable) {
        return commands;
    }

    for (int row = 0; row < m_editorTable->rowCount(); ++row) {
        const auto textAt = [this, row](int column) {
            auto* item = m_editorTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        CommonInterfaceCommand command;
        command.category = textAt(0);
        command.name = textAt(1);
        command.commandText = textAt(2);
        command.doneVariable = textAt(3);
        const QString doneBufferText = textAt(4);

        bool bufferOk = false;
        command.doneBuffer = doneBufferText.isEmpty() ? -1 : doneBufferText.toInt(&bufferOk);
        if (!doneBufferText.isEmpty() && !bufferOk) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("invalid_done_buffer")).arg(row + 1);
            }
            return {};
        }

        const QString doneValueText = textAt(5);
        bool ok = false;
        command.doneValue = doneValueText.isEmpty() ? 1 : doneValueText.toInt(&ok);
        if (!doneValueText.isEmpty() && !ok) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("invalid_done_value")).arg(row + 1);
            }
            return {};
        }

        const bool allEmpty = command.category.isEmpty() && command.name.isEmpty() && command.commandText.isEmpty()
            && command.doneVariable.isEmpty() && doneBufferText.isEmpty() && doneValueText.isEmpty();
        if (allEmpty) {
            continue;
        }

        if (command.category.isEmpty() || command.name.isEmpty() || command.commandText.isEmpty()) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("missing_required_fields")).arg(row + 1);
            }
            return {};
        }

        commands.push_back(command);
    }

    return commands;
}

CommonInterfaceMaterialTransferConfig CommonInterfacePage::collectTransferConfigFromEditor(QString* errorMessage) const
{
    CommonInterfaceMaterialTransferConfig config;
    if (m_editorRobotNameEdit) {
        config.robotName = m_editorRobotNameEdit->text().trimmed();
    }
    if (m_editorMaterialLocationEdit) {
        config.currentMaterialLocation = m_editorMaterialLocationEdit->text().trimmed();
    }
    if (!m_editorTransferTable) {
        return config;
    }

    for (int row = 0; row < m_editorTransferTable->rowCount(); ++row) {
        const auto textAt = [this, row](int column) {
            auto* item = m_editorTransferTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        const QString stationName = textAt(0);
        const QString pickupCommand = textAt(1);
        const QString pickupDoneVariable = textAt(2);
        const QString pickupDoneValueText = textAt(3);
        const QString placeCommand = textAt(4);
        const QString placeDoneVariable = textAt(5);
        const QString placeDoneValueText = textAt(6);
        const QString color = textAt(7);

        const bool allEmpty = stationName.isEmpty() && pickupCommand.isEmpty() && pickupDoneVariable.isEmpty()
            && pickupDoneValueText.isEmpty() && placeCommand.isEmpty() && placeDoneVariable.isEmpty()
            && placeDoneValueText.isEmpty() && color.isEmpty();
        if (allEmpty) {
            continue;
        }

        if (stationName.isEmpty() || pickupCommand.isEmpty() || placeCommand.isEmpty()) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("missing_transfer_fields")).arg(row + 1);
            }
            return {};
        }

        bool pickupDoneValueOk = false;
        const int pickupDoneValue = pickupDoneValueText.isEmpty() ? 1 : pickupDoneValueText.toInt(&pickupDoneValueOk);
        if (!pickupDoneValueText.isEmpty() && !pickupDoneValueOk) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("invalid_done_value")).arg(row + 1);
            }
            return {};
        }

        bool placeDoneValueOk = false;
        const int placeDoneValue = placeDoneValueText.isEmpty() ? 1 : placeDoneValueText.toInt(&placeDoneValueOk);
        if (!placeDoneValueText.isEmpty() && !placeDoneValueOk) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("invalid_done_value")).arg(row + 1);
            }
            return {};
        }

        CommonInterfaceMaterialTransferStation station;
        station.stationName = stationName;
        station.pickupCommand = pickupCommand;
        station.pickupDoneVariable = pickupDoneVariable;
        station.pickupDoneValue = pickupDoneValue;
        station.placeCommand = placeCommand;
        station.placeDoneVariable = placeDoneVariable;
        station.placeDoneValue = placeDoneValue;
        station.color = color.isEmpty() ? QStringLiteral("#d9e8f5") : color;
        config.stations.push_back(station);
    }

    return config;
}

QString CommonInterfacePage::textForKey(const QString& key) const
{
    const bool english = (m_languageCode == QStringLiteral("en-US"));

    if (key == QStringLiteral("title")) return english ? QStringLiteral("Common Interface") : QString::fromUtf8(u8"常用接口");
    if (key == QStringLiteral("description")) return english ? QStringLiteral("Group the most-used controller commands into configurable quick-action panels.")
                                                             : QString::fromUtf8(u8"把最常用的控制器指令整理成可配置的快捷操作面板。");
    if (key == QStringLiteral("settings")) return english ? QStringLiteral("Settings") : QString::fromUtf8(u8"设置");
    if (key == QStringLiteral("material_transfer_title")) return english ? QStringLiteral("Material Transfer") : QString::fromUtf8(u8"物料传递");
    if (key == QStringLiteral("material_transfer_description")) return english ? QStringLiteral("The topology is generated from the configuration file. Click a station node to run pickup and place in sequence.")
                                                                                : QString::fromUtf8(u8"拓扑图会根据配置文件自动生成。点击工位节点后会自动串行执行取料和放料。");
    if (key == QStringLiteral("runtime_log")) return english ? QStringLiteral("Runtime Log") : QString::fromUtf8(u8"运行日志");
    if (key == QStringLiteral("editor_title")) return english ? QStringLiteral("Common Interface Config") : QString::fromUtf8(u8"常用接口配置");
    if (key == QStringLiteral("config_file")) return english ? QStringLiteral("Config File") : QString::fromUtf8(u8"配置文件");
    if (key == QStringLiteral("browse")) return english ? QStringLiteral("Browse") : QString::fromUtf8(u8"浏览");
    if (key == QStringLiteral("add")) return english ? QStringLiteral("Add") : QString::fromUtf8(u8"新增");
    if (key == QStringLiteral("remove")) return english ? QStringLiteral("Remove") : QString::fromUtf8(u8"删除");
    if (key == QStringLiteral("save")) return english ? QStringLiteral("Save") : QString::fromUtf8(u8"保存");
    if (key == QStringLiteral("switch_config")) return english ? QStringLiteral("Switch Config") : QString::fromUtf8(u8"切换配置");
    if (key == QStringLiteral("column_category")) return english ? QStringLiteral("Category") : QString::fromUtf8(u8"分类");
    if (key == QStringLiteral("column_name")) return english ? QStringLiteral("Name") : QString::fromUtf8(u8"名称");
    if (key == QStringLiteral("column_command")) return english ? QStringLiteral("Command") : QString::fromUtf8(u8"指令字符串");
    if (key == QStringLiteral("column_done_variable")) return english ? QStringLiteral("Done Variable") : QString::fromUtf8(u8"完成变量");
    if (key == QStringLiteral("column_done_buffer")) return english ? QStringLiteral("Done Buffer") : QString::fromUtf8(u8"完成变量Buffer");
    if (key == QStringLiteral("column_done_value")) return english ? QStringLiteral("Done Value") : QString::fromUtf8(u8"完成校验值");
    if (key == QStringLiteral("column_station_name")) return english ? QStringLiteral("Station") : QString::fromUtf8(u8"工位名");
    if (key == QStringLiteral("column_pickup_command")) return english ? QStringLiteral("Pickup Command") : QString::fromUtf8(u8"取料指令");
    if (key == QStringLiteral("column_pickup_done_variable")) return english ? QStringLiteral("Pickup Done Variable") : QString::fromUtf8(u8"取料完成变量");
    if (key == QStringLiteral("column_pickup_done_value")) return english ? QStringLiteral("Pickup Done Value") : QString::fromUtf8(u8"取料完成值");
    if (key == QStringLiteral("column_place_command")) return english ? QStringLiteral("Place Command") : QString::fromUtf8(u8"放料指令");
    if (key == QStringLiteral("column_place_done_variable")) return english ? QStringLiteral("Place Done Variable") : QString::fromUtf8(u8"放料完成变量");
    if (key == QStringLiteral("column_place_done_value")) return english ? QStringLiteral("Place Done Value") : QString::fromUtf8(u8"放料完成值");
    if (key == QStringLiteral("column_color")) return english ? QStringLiteral("Node Color") : QString::fromUtf8(u8"节点颜色");
    if (key == QStringLiteral("robot_name")) return english ? QStringLiteral("Robot") : QString::fromUtf8(u8"机械臂");
    if (key == QStringLiteral("material_location")) return english ? QStringLiteral("Material Location") : QString::fromUtf8(u8"物料位置");
    if (key == QStringLiteral("tab_commands")) return english ? QStringLiteral("Common Commands") : QString::fromUtf8(u8"常用指令");
    if (key == QStringLiteral("tab_transfer")) return english ? QStringLiteral("Material Transfer") : QString::fromUtf8(u8"物料传递");
    if (key == QStringLiteral("unknown")) return english ? QStringLiteral("Unknown") : QString::fromUtf8(u8"未指定");
    if (key == QStringLiteral("ready")) return english ? QStringLiteral("Ready") : QString::fromUtf8(u8"就绪");
    if (key == QStringLiteral("save_success")) return english ? QStringLiteral("Configuration saved successfully") : QString::fromUtf8(u8"配置保存成功");
    if (key == QStringLiteral("no_commands")) return english ? QStringLiteral("No common commands were loaded. Use Settings to add command definitions.")
                                                              : QString::fromUtf8(u8"当前没有加载到常用指令，请通过“设置”新增指令配置。");
    if (key == QStringLiteral("not_connected")) return english ? QStringLiteral("Controller is not connected") : QString::fromUtf8(u8"控制器未连接");
    if (key == QStringLiteral("execute_success")) return english ? QStringLiteral("Command completed: %1") : QString::fromUtf8(u8"指令执行完成：%1");
    if (key == QStringLiteral("transfer_success")) return english ? QStringLiteral("Transfer completed: %1") : QString::fromUtf8(u8"搬运执行完成：%1");
    if (key == QStringLiteral("transfer_command_missing")) return english ? QStringLiteral("No transfer action is configured for %1")
                                                                          : QString::fromUtf8(u8"%1 未配置可执行的取放料动作");
    if (key == QStringLiteral("missing_transfer_fields")) return english ? QStringLiteral("Station name, pickup command, and place command are required on transfer row %1")
                                                                         : QString::fromUtf8(u8"物料传递第 %1 行缺少必填项：工位名、取料指令、放料指令");
    if (key == QStringLiteral("wait_timeout")) return english ? QStringLiteral("Timed out waiting for %1 in buffer %2 = %3")
                                                              : QString::fromUtf8(u8"等待 buffer %2 中的 %1 = %3 超时");
    if (key == QStringLiteral("invalid_done_buffer")) return english ? QStringLiteral("Invalid done buffer on row %1")
                                                                     : QString::fromUtf8(u8"第 %1 行 Buffer 编号无效");
    if (key == QStringLiteral("invalid_done_value")) return english ? QStringLiteral("Invalid done value on row %1")
                                                                    : QString::fromUtf8(u8"第 %1 行完成校验值无效");
    if (key == QStringLiteral("missing_required_fields")) return english ? QStringLiteral("Category, name, and command are required on row %1")
                                                                         : QString::fromUtf8(u8"第 %1 行缺少必填项：分类、名称、指令字符串");
    return key;
}
QString CommonInterfacePage::configFilePath() const
{
    const QString relativePath = QStringLiteral("Config/common_interface_simulator.json");

    const QStringList candidatePaths = {
        QDir(QCoreApplication::applicationDirPath()).filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../") + relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../") + relativePath),
        QDir(QDir::currentPath()).filePath(relativePath),
        QDir(QDir::currentPath()).filePath(QStringLiteral("../") + relativePath)
    };

    for (const QString& candidate : candidatePaths) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }

    return QFileInfo(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../") + relativePath)).absoluteFilePath();
}

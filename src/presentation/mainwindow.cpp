#include "presentation/mainwindow.h"
#include "presentation/uihelpers.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QFontMetrics>

namespace
{
constexpr int kIoNameColumnWidth = 150;
constexpr int kIoMiddleSpacingWidth = 20;
constexpr int kIoStatusColumnWidth = 92;
constexpr int kIoRowWidth = kIoNameColumnWidth + kIoMiddleSpacingWidth + kIoStatusColumnWidth + 6;

QString appBasePath()
{
    const QStringList candidates = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath(),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral(".."))
    };

    for (const QString& candidate : candidates) {
        if (QDir(candidate).exists(QStringLiteral("Image")) || QDir(candidate).exists(QStringLiteral("image"))) {
            return candidate;
        }
    }

    return QCoreApplication::applicationDirPath();
}

QString normalizedIoType(const QString& value)
{
    return value.trimmed().toUpper();
}

QStringList ioTypeDisplayOrder()
{
    return QStringList()
        << QStringLiteral("DI")
        << QStringLiteral("DO")
        << QStringLiteral("AI")
        << QStringLiteral("AO");
}

QString trZh(const char* text)
{
    return QString::fromUtf8(text);
}

QString findImageByCandidates(const QStringList& candidates)
{
    const QStringList imageDirs = {
        QDir(appBasePath()).filePath(QStringLiteral("Image")),
        QDir(appBasePath()).filePath(QStringLiteral("image"))
    };

    for (const QString& imageDirPath : imageDirs) {
        QDir imageDir(imageDirPath);
        if (!imageDir.exists()) {
            continue;
        }
        for (const QString& candidate : candidates) {
            const QString path = imageDir.filePath(candidate);
            if (QFileInfo::exists(path)) {
                return path;
            }
        }

        const QStringList files = imageDir.entryList(QStringList() << QStringLiteral("*.png") << QStringLiteral("*.ico"),
                                                     QDir::Files,
                                                     QDir::Name);
        for (const QString& candidate : candidates) {
            for (const QString& fileName : files) {
                if (fileName.compare(candidate, Qt::CaseInsensitive) == 0
                    || fileName.contains(candidate, Qt::CaseInsensitive)) {
                    return imageDir.filePath(fileName);
                }
            }
        }
    }

    return QString();
}

QTableWidgetItem* makeTableItem(const QString& text)
{
    return new QTableWidgetItem(text);
}
}

MainWindow::MainWindow(QApplication& application, ControllerService& controllerService, QWidget* parent)
    : QWidget(parent)
    , m_application(application)
    , m_controllerService(controllerService)
    , m_logger(FileLogger::defaultLogDirectoryPath())
    , m_configFilePath(QDir(appBasePath()).filePath(QStringLiteral("Config/io_config_simulator.json")))
{
    m_indicatorOff.load(findImageByCandidates(QStringList() << QStringLiteral("io1.png") << QStringLiteral("(1).png")));
    m_indicatorOn.load(findImageByCandidates(QStringList() << QStringLiteral("IO2.png") << QStringLiteral("io2.png") << QStringLiteral("(2).png")));
    m_indicatorError.load(findImageByCandidates(QStringList() << QStringLiteral("IO3.png") << QStringLiteral("io3.png") << QStringLiteral("copy-copy.png")));

    buildUi();
    loadIoConfig();
    rebuildModuleGroups();
    populateEditorTable();
    updateIoControlsEnabled(false);
    retranslateUi();
    logMessage(QStringLiteral("IO tester started"));
}

void MainWindow::setLanguage(const QString& languageCode)
{
    const QString normalized = languageCode.trimmed().compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
                                   ? QStringLiteral("en-US")
                                   : QStringLiteral("zh-CN");
    if (m_languageCode == normalized) {
        return;
    }

    m_languageCode = normalized;
    retranslateUi();
    rebuildModuleGroups();
    populateEditorTable();
    updateIoControlsEnabled(m_controllerService.isConnected());
    if (m_controllerService.isConnected()) {
        refreshIoStateInternal(false);
    }
}

void MainWindow::setControllerConnected(bool connected)
{
    updateIoControlsEnabled(connected);
    if (connected) {
        if (m_pollTimer) {
            m_pollTimer->start();
        }
        refreshIoStateInternal(false);
    } else if (m_pollTimer) {
        m_pollTimer->stop();
    }
}

void MainWindow::setPollIntervalMs(int value)
{
    if (m_pollTimer) {
        m_pollTimer->setInterval(value);
    }
    if (m_pollIntervalEdit && m_pollIntervalEdit->value() != value) {
        QSignalBlocker blocker(m_pollIntervalEdit);
        m_pollIntervalEdit->setValue(value);
    }
}

int MainWindow::pollIntervalMs() const
{
    return m_pollIntervalEdit ? m_pollIntervalEdit->value() : 100;
}

bool MainWindow::refreshNow(QString* summary)
{
    return refreshIoStateInternal(true, summary);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    const int columns = calculateIoColumns();
    if (columns != m_currentIoColumns) {
        rebuildModuleGroups();
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    const int columns = calculateIoColumns();
    if (columns != m_currentIoColumns) {
        rebuildModuleGroups();
    }
}

QString MainWindow::textForKey(const QString& key) const
{
    const bool english = (m_languageCode == QStringLiteral("en-US"));

    if (key == QStringLiteral("ready")) return english ? QStringLiteral("Ready") : trZh("就绪");
    if (key == QStringLiteral("io_overview")) return english ? QStringLiteral("Module IO Overview") : trZh("模块 IO 总览");
    if (key == QStringLiteral("settings")) return english ? QStringLiteral("Settings") : trZh("设置");
    if (key == QStringLiteral("config_editor")) return english ? QStringLiteral("IO Config Editor") : trZh("IO 配置编辑");
    if (key == QStringLiteral("config_file")) return english ? QStringLiteral("Config File") : trZh("配置文件");
    if (key == QStringLiteral("browse")) return english ? QStringLiteral("Browse") : trZh("浏览");
    if (key == QStringLiteral("add")) return english ? QStringLiteral("Add") : trZh("新增");
    if (key == QStringLiteral("remove")) return english ? QStringLiteral("Remove") : trZh("删除");
    if (key == QStringLiteral("save")) return english ? QStringLiteral("Save") : trZh("保存");
    if (key == QStringLiteral("runtime_log")) return english ? QStringLiteral("Runtime Log") : trZh("运行日志");
    if (key == QStringLiteral("no_io_points")) return english ? QStringLiteral("No IO points were loaded from the current config.")
                                                               : trZh("当前配置中没有加载到任何 IO 点位。");
    if (key == QStringLiteral("unsupported")) return english ? QStringLiteral("Unsupported") : trZh("暂不支持");
    if (key == QStringLiteral("enter_value")) return english ? QStringLiteral("Enter value") : trZh("输入数值");
    if (key == QStringLiteral("unknown")) return english ? QStringLiteral("Unknown") : trZh("未分组");
    if (key == QStringLiteral("off")) return english ? QStringLiteral("OFF") : trZh("关");
    if (key == QStringLiteral("on")) return english ? QStringLiteral("ON") : trZh("开");
    if (key == QStringLiteral("action_refresh")) return english ? QStringLiteral("Refresh") : trZh("刷新");
    if (key == QStringLiteral("action_do")) return english ? QStringLiteral("DO Switch") : trZh("DO 开关");
    if (key == QStringLiteral("action_ao")) return english ? QStringLiteral("AO Write") : trZh("AO 写入");
    if (key == QStringLiteral("action_reload")) return english ? QStringLiteral("IO Config Reload") : trZh("IO 配置热加载");
    if (key == QStringLiteral("action_save_config")) return english ? QStringLiteral("Save Config") : trZh("保存配置");
    if (key == QStringLiteral("action_switch_config")) return english ? QStringLiteral("Switch Config") : trZh("切换配置");
    if (key == QStringLiteral("not_connected")) return english ? QStringLiteral("Controller is not connected") : trZh("控制器未连接");
    if (key == QStringLiteral("reload_success")) return english ? QStringLiteral("Configuration hot reloaded") : trZh("配置已热加载");
    if (key == QStringLiteral("save_success")) return english ? QStringLiteral("Configuration saved successfully") : trZh("配置保存成功");
    if (key == QStringLiteral("invalid_number")) return english ? QStringLiteral("Invalid numeric value: %1") : trZh("无效的数值输入: %1");
    if (key == QStringLiteral("refresh_summary")) return english ? QStringLiteral("Refresh completed: %1 / %2 variables readable")
                                                                  : trZh("刷新完成: %1 / %2 个变量可读");
    if (key == QStringLiteral("config_removed")) return english ? QStringLiteral("IO config file was removed: %1")
                                                                 : trZh("IO 配置文件已被移除: %1");
    if (key == QStringLiteral("config_save_failed")) return english ? QStringLiteral("Failed to save config: %1")
                                                                     : trZh("配置保存失败: %1");
    if (key == QStringLiteral("config_switch_failed")) return english ? QStringLiteral("Failed to switch config: %1")
                                                                       : trZh("配置切换失败: %1");
    if (key == QStringLiteral("missing_required_fields")) return english ? QStringLiteral("Name, variable, module, and IO type are required on row %1")
                                                                          : trZh("第 %1 行缺少必填字段：名称、变量名、模块、IO 类型");
    if (key == QStringLiteral("table_name")) return english ? QStringLiteral("Name") : trZh("名称");
    if (key == QStringLiteral("table_variable")) return english ? QStringLiteral("Variable") : trZh("变量名");
    if (key == QStringLiteral("table_module")) return english ? QStringLiteral("Module") : trZh("模块");
    if (key == QStringLiteral("table_type")) return english ? QStringLiteral("IO Type") : trZh("IO 类型");
    if (key == QStringLiteral("table_unit")) return english ? QStringLiteral("Unit") : trZh("单位");
    if (key == QStringLiteral("table_range")) return english ? QStringLiteral("Range") : trZh("量程");
    if (key == QStringLiteral("table_description")) return english ? QStringLiteral("Description") : trZh("说明");
    if (key == QStringLiteral("default_name")) return english ? QStringLiteral("New IO") : trZh("新 IO");
    if (key == QStringLiteral("failed_suffix")) return english ? QStringLiteral("%1 Failed") : trZh("%1失败");
    return key;
}

int MainWindow::calculateIoColumns() const
{
    constexpr int kMaxColumns = 3;
    constexpr int kMinColumns = 1;
    constexpr int kPreferredItemWidth = 240;
    constexpr int kColumnSpacing = 28;

    int availableWidth = width();
    if (m_ioScrollArea && m_ioScrollArea->viewport()) {
        availableWidth = m_ioScrollArea->viewport()->width();
    } else if (m_ioContainer) {
        availableWidth = m_ioContainer->width();
    }

    const int columns = (availableWidth + kColumnSpacing) / (kPreferredItemWidth + kColumnSpacing);
    return qBound(kMinColumns, columns, kMaxColumns);
}

void MainWindow::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(12);

    m_feedbackLabel = new QLabel(this);
    m_feedbackLabel->setWordWrap(true);
    m_settingsButton = new QPushButton(this);
    m_pollLabel = new QLabel(this);
    m_pollIntervalEdit = new QSpinBox(this);
    m_pollIntervalEdit->setRange(20, 5000);
    m_pollIntervalEdit->setValue(100);
    m_settingsButton->setStyleSheet(UiHelpers::secondaryButtonStyle());

    auto* topRowLayout = new QHBoxLayout();
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(10);
    topRowLayout->addWidget(m_feedbackLabel, 1);
    topRowLayout->addWidget(m_pollLabel, 0, Qt::AlignVCenter);
    topRowLayout->addWidget(m_pollIntervalEdit, 0, Qt::AlignVCenter);
    topRowLayout->addWidget(m_settingsButton, 0, Qt::AlignTop);

    auto* ioPanel = new QWidget(this);
    auto* ioLayout = new QVBoxLayout(ioPanel);
    ioLayout->setContentsMargins(0, 0, 0, 0);
    ioLayout->setSpacing(10);

    auto* ioHeaderLayout = new QHBoxLayout();
    m_ioOverviewLabel = new QLabel(ioPanel);
    ioHeaderLayout->addWidget(m_ioOverviewLabel);
    ioHeaderLayout->addStretch();

    m_ioScrollArea = new QScrollArea(ioPanel);
    m_ioScrollArea->setWidgetResizable(true);
    m_ioContainer = new QWidget(m_ioScrollArea);
    m_ioContainerLayout = new QVBoxLayout(m_ioContainer);
    m_ioContainerLayout->setContentsMargins(8, 8, 8, 8);
    m_ioContainerLayout->setSpacing(12);
    m_ioScrollArea->setWidget(m_ioContainer);

    ioLayout->addLayout(ioHeaderLayout);
    ioLayout->addWidget(m_ioScrollArea, 1);

    m_configEditorDialog = new QDialog(this);
    m_configEditorDialog->setModal(false);
    auto* dialogLayout = new QVBoxLayout(m_configEditorDialog);
    dialogLayout->setContentsMargins(16, 16, 16, 16);

    m_editorPanel = new QWidget(m_configEditorDialog);
    auto* editorLayout = new QVBoxLayout(m_editorPanel);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(10);

    m_configEditorLabel = new QLabel(m_editorPanel);

    auto* fileRow = new QHBoxLayout();
    m_configFileLabel = new QLabel(m_editorPanel);
    m_configFileEdit = new QLineEdit(m_editorPanel);
    m_configFileEdit->setReadOnly(true);
    m_browseConfigButton = new QPushButton(m_editorPanel);
    fileRow->addWidget(m_configFileLabel);
    fileRow->addWidget(m_configFileEdit, 1);
    fileRow->addWidget(m_browseConfigButton);

    auto* actionRow = new QHBoxLayout();
    m_addIoButton = new QPushButton(m_editorPanel);
    m_removeIoButton = new QPushButton(m_editorPanel);
    m_saveConfigButton = new QPushButton(m_editorPanel);
    actionRow->addWidget(m_addIoButton);
    actionRow->addWidget(m_removeIoButton);
    actionRow->addWidget(m_saveConfigButton);
    actionRow->addStretch();

    m_configTable = new QTableWidget(m_editorPanel);
    m_configTable->setColumnCount(7);
    m_configTable->horizontalHeader()->setStretchLastSection(true);
    m_configTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_configTable->verticalHeader()->setVisible(false);
    m_configTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_configTable->setSelectionMode(QAbstractItemView::SingleSelection);

    editorLayout->addWidget(m_configEditorLabel);
    editorLayout->addLayout(fileRow);
    editorLayout->addLayout(actionRow);
    editorLayout->addWidget(m_configTable, 1);
    dialogLayout->addWidget(m_editorPanel);

    m_runtimeLogLabel = new QLabel(this);
    m_runtimeLogLabel->hide();
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(180);

    rootLayout->addLayout(topRowLayout);
    rootLayout->addWidget(ioPanel, 1);
    rootLayout->addWidget(m_logView);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(100);

    m_configWatcher = new QFileSystemWatcher(this);
    m_configWatcher->addPath(m_configFilePath);

    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::refreshIoState);
    connect(m_configWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::handleConfigFileChanged);
    connect(m_browseConfigButton, &QPushButton::clicked, this, &MainWindow::browseConfigFile);
    connect(m_saveConfigButton, &QPushButton::clicked, this, &MainWindow::saveIoConfig);
    connect(m_addIoButton, &QPushButton::clicked, this, &MainWindow::addIoPoint);
    connect(m_removeIoButton, &QPushButton::clicked, this, &MainWindow::removeSelectedIoPoint);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openConfigEditorDialog);
    connect(m_pollIntervalEdit, qOverload<int>(&QSpinBox::valueChanged), this, &MainWindow::handlePollIntervalChanged);
}

void MainWindow::retranslateUi()
{
    if (m_ioOverviewLabel) m_ioOverviewLabel->setText(textForKey(QStringLiteral("io_overview")));
    if (m_pollLabel) m_pollLabel->setText(m_languageCode == QStringLiteral("en-US")
                                              ? QStringLiteral("Poll")
                                              : QString::fromWCharArray(L"轮询周期"));
    if (m_pollIntervalEdit) m_pollIntervalEdit->setSuffix(QStringLiteral(" ms"));
    if (m_settingsButton) m_settingsButton->setText(textForKey(QStringLiteral("settings")));
    if (m_configEditorLabel) m_configEditorLabel->setText(textForKey(QStringLiteral("config_editor")));
    if (m_configFileLabel) m_configFileLabel->setText(textForKey(QStringLiteral("config_file")));
    if (m_browseConfigButton) m_browseConfigButton->setText(textForKey(QStringLiteral("browse")));
    if (m_addIoButton) m_addIoButton->setText(textForKey(QStringLiteral("add")));
    if (m_removeIoButton) m_removeIoButton->setText(textForKey(QStringLiteral("remove")));
    if (m_saveConfigButton) m_saveConfigButton->setText(textForKey(QStringLiteral("save")));
    if (m_runtimeLogLabel) m_runtimeLogLabel->setText(textForKey(QStringLiteral("runtime_log")));

    if (m_configTable) {
        QStringList headers;
        headers << textForKey(QStringLiteral("table_name"))
                << textForKey(QStringLiteral("table_variable"))
                << textForKey(QStringLiteral("table_module"))
                << textForKey(QStringLiteral("table_type"))
                << textForKey(QStringLiteral("table_unit"))
                << textForKey(QStringLiteral("table_range"))
                << textForKey(QStringLiteral("table_description"));
        m_configTable->setHorizontalHeaderLabels(headers);
    }

    if (m_feedbackLabel && m_feedbackLabel->text().isEmpty()) {
        m_feedbackLabel->setText(textForKey(QStringLiteral("ready")));
        m_feedbackLabel->setStyleSheet(UiHelpers::statusStyle(QStringLiteral("info")));
    }

    if (m_configFileEdit) {
        m_configFileEdit->setText(m_configFilePath);
    }

    if (m_configEditorDialog) {
        m_configEditorDialog->setWindowTitle(textForKey(QStringLiteral("config_editor")));
        m_configEditorDialog->resize(920, 640);
    }
}

void MainWindow::loadIoConfig()
{
    QString errorMessage;
    m_points = m_ioConfigurationService.loadPoints(m_configFilePath, &errorMessage);
    if (!errorMessage.isEmpty()) {
        logMessage(errorMessage);
    }
    logMessage(QStringLiteral("Loaded IO config from %1, total points: %2").arg(m_configFilePath).arg(m_points.size()));
}

void MainWindow::rebuildModuleGroups()
{
    m_currentIoColumns = calculateIoColumns();

    while (m_ioContainerLayout->count() > 0) {
        QLayoutItem* item = m_ioContainerLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    m_bindings.clear();

    QMap<QString, QList<IoPoint>> pointsByModule;
    for (const IoPoint& point : m_points) {
        pointsByModule[point.module].push_back(point);
    }

    for (auto it = pointsByModule.cbegin(); it != pointsByModule.cend(); ++it) {
        auto* groupBox = new QGroupBox(it.key().isEmpty() ? textForKey(QStringLiteral("unknown")) : it.key(), m_ioContainer);
        auto* groupLayout = new QGridLayout(groupBox);
        groupLayout->setContentsMargins(12, 12, 12, 12);
        groupLayout->setHorizontalSpacing(0);
        groupLayout->setVerticalSpacing(8);

        const QList<IoPoint>& modulePoints = it.value();
        for (int column = 0; column < m_currentIoColumns; ++column) {
            const int contentColumn = column * 2;
            groupLayout->setColumnMinimumWidth(contentColumn, kIoRowWidth);
            groupLayout->setColumnStretch(contentColumn, 0);
            if (column < m_currentIoColumns - 1) {
                const int gapColumn = contentColumn + 1;
                groupLayout->setColumnMinimumWidth(gapColumn, 28);
                groupLayout->setColumnStretch(gapColumn, 0);
            }
        }
        groupLayout->setColumnStretch(m_currentIoColumns * 2, 1);

        QMap<QString, QList<IoPoint>> pointsByType;
        for (const IoPoint& point : modulePoints) {
            pointsByType[normalizedIoType(point.ioType)].push_back(point);
        }

        int currentRow = 0;
        const QStringList orderedTypes = ioTypeDisplayOrder();
        for (const QString& ioType : orderedTypes) {
            const QList<IoPoint> typedPoints = pointsByType.take(ioType);
            if (typedPoints.isEmpty()) {
                continue;
            }

            for (int index = 0; index < typedPoints.size(); ++index) {
                const int row = currentRow + (index / m_currentIoColumns);
                const int column = index % m_currentIoColumns;
                const int gridColumn = column * 2;
                QWidget* rowWidget = createPointRow(typedPoints.at(index));
                groupLayout->addWidget(rowWidget, row, gridColumn, Qt::AlignLeft | Qt::AlignVCenter);
            }

            currentRow += (typedPoints.size() + m_currentIoColumns - 1) / m_currentIoColumns;
        }

        for (auto remainingIt = pointsByType.cbegin(); remainingIt != pointsByType.cend(); ++remainingIt) {
            const QList<IoPoint>& typedPoints = remainingIt.value();
            if (typedPoints.isEmpty()) {
                continue;
            }

            for (int index = 0; index < typedPoints.size(); ++index) {
                const int row = currentRow + (index / m_currentIoColumns);
                const int column = index % m_currentIoColumns;
                const int gridColumn = column * 2;
                QWidget* rowWidget = createPointRow(typedPoints.at(index));
                groupLayout->addWidget(rowWidget, row, gridColumn, Qt::AlignLeft | Qt::AlignVCenter);
            }

            currentRow += (typedPoints.size() + m_currentIoColumns - 1) / m_currentIoColumns;
        }

        m_ioContainerLayout->addWidget(groupBox);
    }

    if (pointsByModule.isEmpty()) {
        auto* emptyLabel = new QLabel(textForKey(QStringLiteral("no_io_points")), m_ioContainer);
        emptyLabel->setWordWrap(true);
        m_ioContainerLayout->addWidget(emptyLabel);
    }

    m_ioContainerLayout->addStretch();
}

void MainWindow::populateEditorTable()
{
    if (!m_configTable) {
        return;
    }

    QSignalBlocker blocker(m_configTable);
    m_configTable->setRowCount(m_points.size());

    for (int row = 0; row < m_points.size(); ++row) {
        const IoPoint& point = m_points.at(row);
        m_configTable->setItem(row, 0, makeTableItem(point.name));
        m_configTable->setItem(row, 1, makeTableItem(point.variableName));
        m_configTable->setItem(row, 2, makeTableItem(point.module));
        m_configTable->setItem(row, 3, makeTableItem(point.ioType));
        m_configTable->setItem(row, 4, makeTableItem(point.unit));
        m_configTable->setItem(row, 5, makeTableItem(point.range));
        m_configTable->setItem(row, 6, makeTableItem(point.description));
    }

    if (m_configFileEdit) {
        m_configFileEdit->setText(m_configFilePath);
    }
}

QVector<IoPoint> MainWindow::collectPointsFromEditor(QString* errorMessage) const
{
    QVector<IoPoint> points;
    if (!m_configTable) {
        return points;
    }

    for (int row = 0; row < m_configTable->rowCount(); ++row) {
        auto itemText = [this, row](int column) -> QString {
            auto* item = m_configTable->item(row, column);
            return item ? item->text().trimmed() : QString();
        };

        IoPoint point;
        point.name = itemText(0);
        point.variableName = itemText(1);
        point.module = itemText(2);
        point.ioType = itemText(3).toUpper();
        point.unit = itemText(4);
        point.range = itemText(5);
        point.description = itemText(6);
        point.id = point.variableName;
        point.direction = (point.ioType == QStringLiteral("DI") || point.ioType == QStringLiteral("AI"))
                              ? QStringLiteral("input")
                              : QStringLiteral("output");

        const bool allEmpty = point.name.isEmpty() && point.variableName.isEmpty() && point.module.isEmpty()
            && point.ioType.isEmpty() && point.unit.isEmpty() && point.range.isEmpty() && point.description.isEmpty();
        if (allEmpty) {
            continue;
        }

        if (point.name.isEmpty() || point.variableName.isEmpty() || point.module.isEmpty() || point.ioType.isEmpty()) {
            if (errorMessage) {
                *errorMessage = textForKey(QStringLiteral("missing_required_fields")).arg(row + 1);
            }
            return {};
        }

        points.push_back(point);
    }

    return points;
}

void MainWindow::applyConfigFilePath(const QString& filePath)
{
    m_configFilePath = filePath;
    updateWatcherPath(filePath);
    if (m_configFileEdit) {
        m_configFileEdit->setText(filePath);
    }
}

void MainWindow::updateWatcherPath(const QString& filePath)
{
    if (!m_configWatcher) {
        return;
    }

    const QStringList files = m_configWatcher->files();
    for (const QString& existing : files) {
        m_configWatcher->removePath(existing);
    }
    if (!filePath.isEmpty()) {
        m_configWatcher->addPath(filePath);
    }
}

void MainWindow::updateIoControlsEnabled(bool connected)
{
    for (IoWidgetBinding& binding : m_bindings) {
        if (binding.doSwitch) {
            binding.doSwitch->setEnabled(connected);
        }
        if (binding.aoInput) {
            binding.aoInput->setEnabled(connected);
        }
    }
}

bool MainWindow::refreshIoStateInternal(bool reportManual, QString* summary)
{
    if (!m_controllerService.isConnected()) {
        if (reportManual) {
            reportActionResult(textForKey(QStringLiteral("action_refresh")), false, textForKey(QStringLiteral("not_connected")), false);
        }
        if (summary) {
            *summary = textForKey(QStringLiteral("not_connected"));
        }
        return false;
    }

    int successCount = 0;
    for (IoWidgetBinding& binding : m_bindings) {
        QString errorMessage;
        const QString ioType = normalizedIoType(binding.point.ioType);

        if (ioType == QStringLiteral("DI")) {
            int value = 0;
            if (m_controllerService.readIntegerVariable(binding.point.variableName, &value, &errorMessage)) {
                updateIndicator(binding.indicatorLabel, value);
                ++successCount;
            } else {
                updateIndicatorError(binding.indicatorLabel);
            }
        } else if (ioType == QStringLiteral("DO")) {
            int value = 0;
            if (m_controllerService.readIntegerVariable(binding.point.variableName, &value, &errorMessage)) {
                if (binding.doSwitch) {
                    QSignalBlocker blocker(binding.doSwitch);
                    const bool checked = (value != 0);
                    binding.doSwitch->setChecked(checked);
                    binding.doSwitch->setText(checked ? textForKey(QStringLiteral("on")) : textForKey(QStringLiteral("off")));
                }
                ++successCount;
            }
        } else if (ioType == QStringLiteral("AI")) {
            double value = 0.0;
            if (m_controllerService.readRealVariable(binding.point.variableName, &value, &errorMessage)) {
                if (binding.aiDisplay) {
                    binding.aiDisplay->setText(QString::number(value, 'f', 3));
                }
                ++successCount;
            } else if (binding.aiDisplay) {
                binding.aiDisplay->setText(QStringLiteral("ERR"));
            }
        } else if (ioType == QStringLiteral("AO")) {
            double value = 0.0;
            if (m_controllerService.readRealVariable(binding.point.variableName, &value, &errorMessage)) {
                if (binding.aoInput && !binding.aoInput->hasFocus()) {
                    binding.aoInput->setText(QString::number(value, 'f', 3));
                }
                ++successCount;
            } else if (binding.aoInput && !binding.aoInput->hasFocus()) {
                binding.aoInput->setText(QStringLiteral("ERR"));
            }
        }
    }

    const QString localSummary = textForKey(QStringLiteral("refresh_summary")).arg(successCount).arg(m_bindings.size());
    if (reportManual || m_lastAutoRefreshSummary != localSummary) {
        logMessage(localSummary);
        m_lastAutoRefreshSummary = localSummary;
    }
    if (summary) {
        *summary = localSummary;
    }
    if (reportManual) {
        reportActionResult(textForKey(QStringLiteral("action_refresh")), successCount == m_bindings.size(), localSummary, successCount != m_bindings.size());
    }
    return successCount == m_bindings.size();
}

void MainWindow::refreshIoState()
{
    refreshIoStateInternal(false);
}

QWidget* MainWindow::createPointRow(const IoPoint& point)
{
    auto* rowWidget = new QWidget(m_ioContainer);
    rowWidget->setFixedWidth(kIoRowWidth);
    rowWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto* rowLayout = new QGridLayout(rowWidget);
    rowLayout->setContentsMargins(0, 2, 0, 2);
    rowLayout->setHorizontalSpacing(0);
    rowLayout->setVerticalSpacing(0);
    rowLayout->setColumnMinimumWidth(0, kIoNameColumnWidth);
    rowLayout->setColumnMinimumWidth(1, kIoMiddleSpacingWidth);
    rowLayout->setColumnMinimumWidth(2, kIoStatusColumnWidth);
    rowLayout->setColumnStretch(0, 0);
    rowLayout->setColumnStretch(1, 0);
    rowLayout->setColumnStretch(2, 0);

    const QString displayName = point.name.isEmpty() ? point.variableName : point.name;
    auto* nameLabel = new QLabel(rowWidget);
    nameLabel->setFixedWidth(kIoNameColumnWidth);
    nameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    nameLabel->setWordWrap(false);
    nameLabel->setText(QFontMetrics(nameLabel->font()).elidedText(displayName, Qt::ElideRight, kIoNameColumnWidth - 4));
    nameLabel->setToolTip(point.description.isEmpty() ? displayName : point.description);
    rowLayout->addWidget(nameLabel, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);

    auto* statusContainer = new QWidget(rowWidget);
    auto* statusLayout = new QHBoxLayout(statusContainer);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(0);
    statusContainer->setFixedWidth(kIoStatusColumnWidth);

    IoWidgetBinding binding;
    binding.point = point;

    const QString ioType = normalizedIoType(point.ioType);
    if (ioType == QStringLiteral("DI")) {
        auto* indicatorLabel = new QLabel(rowWidget);
        indicatorLabel->setFixedSize(22, 22);
        updateIndicator(indicatorLabel, IndicatorState::Off);
        statusLayout->addStretch(1);
        statusLayout->addWidget(indicatorLabel, 0, Qt::AlignCenter);
        statusLayout->addStretch(1);
        binding.indicatorLabel = indicatorLabel;
    } else if (ioType == QStringLiteral("DO")) {
        auto* doSwitch = new QCheckBox(rowWidget);
        doSwitch->setText(textForKey(QStringLiteral("off")));
        doSwitch->setFixedWidth(kIoStatusColumnWidth);
        doSwitch->setProperty("variableName", point.variableName);
        doSwitch->setStyleSheet(
            QStringLiteral(
                "QCheckBox::indicator{width:42px;height:22px;}"
                "QCheckBox::indicator:unchecked{border:1px solid #8a8a8a;border-radius:11px;background:#c8c8c8;}"
                "QCheckBox::indicator:checked{border:1px solid #3f8f3f;border-radius:11px;background:#41c64f;}"));
        connect(doSwitch, &QCheckBox::toggled, this, &MainWindow::handleDoSwitchToggled);
        statusLayout->addWidget(doSwitch, 0, Qt::AlignCenter);
        binding.doSwitch = doSwitch;
    } else if (ioType == QStringLiteral("AI")) {
        auto* aiDisplay = new QLineEdit(rowWidget);
        aiDisplay->setReadOnly(true);
        aiDisplay->setFixedWidth(84);
        aiDisplay->setText(QStringLiteral("--"));
        statusLayout->addWidget(aiDisplay, 0, Qt::AlignCenter);
        binding.aiDisplay = aiDisplay;
    } else if (ioType == QStringLiteral("AO")) {
        auto* aoInput = new QLineEdit(rowWidget);
        aoInput->setFixedWidth(84);
        aoInput->setPlaceholderText(point.unit.isEmpty() ? textForKey(QStringLiteral("enter_value")) : point.unit);
        aoInput->setProperty("variableName", point.variableName);
        connect(aoInput, &QLineEdit::editingFinished, this, &MainWindow::handleAoEditingFinished);
        statusLayout->addWidget(aoInput, 0, Qt::AlignCenter);
        binding.aoInput = aoInput;
    } else {
        auto* unsupportedLabel = new QLabel(textForKey(QStringLiteral("unsupported")), rowWidget);
        unsupportedLabel->setFixedWidth(kIoStatusColumnWidth);
        statusLayout->addWidget(unsupportedLabel, 0, Qt::AlignCenter);
    }

    rowLayout->addWidget(statusContainer, 0, 2, Qt::AlignCenter);

    m_bindings.push_back(binding);
    return rowWidget;
}

void MainWindow::handleDoSwitchToggled(bool checked)
{
    auto* doSwitch = qobject_cast<QCheckBox*>(sender());
    if (!doSwitch) {
        return;
    }

    if (!m_controllerService.isConnected()) {
        reportActionResult(textForKey(QStringLiteral("action_do")), false, textForKey(QStringLiteral("not_connected")), false);
        return;
    }

    QString errorMessage;
    const QString variableName = doSwitch->property("variableName").toString();
    if (!m_controllerService.writeIntegerVariable(variableName, checked ? 1 : 0, &errorMessage)) {
        const QSignalBlocker blocker(doSwitch);
        doSwitch->setChecked(!checked);
        reportActionResult(textForKey(QStringLiteral("action_do")), false, QStringLiteral("%1: %2").arg(variableName, errorMessage));
        return;
    }

    reportActionResult(textForKey(QStringLiteral("action_do")), true, QStringLiteral("%1 set to %2").arg(variableName, checked ? QStringLiteral("1") : QStringLiteral("0")), false);
}

void MainWindow::handleAoEditingFinished()
{
    auto* aoInput = qobject_cast<QLineEdit*>(sender());
    if (!aoInput) {
        return;
    }

    if (!m_controllerService.isConnected()) {
        reportActionResult(textForKey(QStringLiteral("action_ao")), false, textForKey(QStringLiteral("not_connected")), false);
        return;
    }

    bool ok = false;
    const double value = aoInput->text().trimmed().toDouble(&ok);
    if (!ok) {
        reportActionResult(textForKey(QStringLiteral("action_ao")), false, textForKey(QStringLiteral("invalid_number")).arg(aoInput->text()), false);
        return;
    }

    QString errorMessage;
    const QString variableName = aoInput->property("variableName").toString();
    if (!m_controllerService.writeRealVariable(variableName, value, &errorMessage)) {
        reportActionResult(textForKey(QStringLiteral("action_ao")), false, QStringLiteral("%1: %2").arg(variableName, errorMessage));
        return;
    }

    reportActionResult(textForKey(QStringLiteral("action_ao")), true, QStringLiteral("%1 set to %2").arg(variableName, QString::number(value, 'f', 3)), false);
}

void MainWindow::handleConfigFileChanged(const QString& path)
{
    if (!QFileInfo::exists(path)) {
        logMessage(textForKey(QStringLiteral("config_removed")).arg(path));
        return;
    }

    if (m_configWatcher && !m_configWatcher->files().contains(path)) {
        m_configWatcher->addPath(path);
    }

    QTimer::singleShot(150, this, &MainWindow::reloadIoConfigFromFile);
}

void MainWindow::reloadIoConfigFromFile()
{
    loadIoConfig();
    rebuildModuleGroups();
    populateEditorTable();
    updateIoControlsEnabled(m_controllerService.isConnected());
    reportActionResult(textForKey(QStringLiteral("action_reload")), true, textForKey(QStringLiteral("reload_success")), false);
    if (m_controllerService.isConnected()) {
        refreshIoStateInternal(false);
    }
}

void MainWindow::browseConfigFile()
{
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          textForKey(QStringLiteral("action_switch_config")),
                                                          QFileInfo(m_configFilePath).absolutePath(),
                                                          QStringLiteral("JSON Files (*.json);;All Files (*.*)"));
    if (filePath.isEmpty()) {
        return;
    }

    applyConfigFilePath(filePath);
    QString errorMessage;
    const QVector<IoPoint> points = m_ioConfigurationService.loadPoints(m_configFilePath, &errorMessage);
    if (!errorMessage.isEmpty() && points.isEmpty()) {
        reportActionResult(textForKey(QStringLiteral("action_switch_config")), false,
                           textForKey(QStringLiteral("config_switch_failed")).arg(errorMessage));
        return;
    }

    m_points = points;
    rebuildModuleGroups();
    populateEditorTable();
    updateIoControlsEnabled(m_controllerService.isConnected());
    reportActionResult(textForKey(QStringLiteral("action_switch_config")), true, m_configFilePath, false);
}

void MainWindow::saveIoConfig()
{
    QString errorMessage;
    const QVector<IoPoint> points = collectPointsFromEditor(&errorMessage);
    if (!errorMessage.isEmpty()) {
        reportActionResult(textForKey(QStringLiteral("action_save_config")), false, errorMessage, false);
        return;
    }

    if (!m_ioConfigurationService.savePoints(m_configFilePath, points, &errorMessage)) {
        reportActionResult(textForKey(QStringLiteral("action_save_config")), false,
                           textForKey(QStringLiteral("config_save_failed")).arg(errorMessage));
        return;
    }

    m_points = points;
    rebuildModuleGroups();
    updateIoControlsEnabled(m_controllerService.isConnected());
    reportActionResult(textForKey(QStringLiteral("action_save_config")), true, textForKey(QStringLiteral("save_success")), false);
    if (m_controllerService.isConnected()) {
        refreshIoStateInternal(false);
    }
}

void MainWindow::addIoPoint()
{
    if (!m_configTable) {
        return;
    }

    const int row = m_configTable->rowCount();
    m_configTable->insertRow(row);
    m_configTable->setItem(row, 0, makeTableItem(textForKey(QStringLiteral("default_name"))));
    m_configTable->setItem(row, 1, makeTableItem(QStringLiteral("new_io_%1").arg(row + 1)));
    m_configTable->setItem(row, 2, makeTableItem(QStringLiteral("general")));
    m_configTable->setItem(row, 3, makeTableItem(QStringLiteral("DI")));
    m_configTable->setItem(row, 4, makeTableItem(QString()));
    m_configTable->setItem(row, 5, makeTableItem(QString()));
    m_configTable->setItem(row, 6, makeTableItem(QString()));
    m_configTable->setCurrentCell(row, 0);
}

void MainWindow::removeSelectedIoPoint()
{
    if (!m_configTable) {
        return;
    }

    const int row = m_configTable->currentRow();
    if (row >= 0) {
        m_configTable->removeRow(row);
    }
}

void MainWindow::openConfigEditorDialog()
{
    if (!m_configEditorDialog) {
        return;
    }

    populateEditorTable();
    m_configEditorDialog->show();
    m_configEditorDialog->raise();
    m_configEditorDialog->activateWindow();
}

void MainWindow::handlePollIntervalChanged(int value)
{
    setPollIntervalMs(value);
}

void MainWindow::logMessage(const QString& message)
{
    const QString line = QStringLiteral("[%1] %2")
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz")))
                             .arg(message);
    m_logView->append(line);
    m_logger.write(FileLogger::Category::Io, message);
}

QPixmap MainWindow::scaledIndicatorPixmap(const QPixmap& pixmap) const
{
    return pixmap.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void MainWindow::updateIndicator(QLabel* label, int value)
{
    updateIndicator(label, value != 0 ? IndicatorState::On : IndicatorState::Off);
}

void MainWindow::updateIndicatorError(QLabel* label)
{
    updateIndicator(label, IndicatorState::Error);
}

void MainWindow::updateIndicator(QLabel* label, IndicatorState state)
{
    if (!label) {
        return;
    }

    QPixmap pixmap;
    if (state == IndicatorState::On) {
        pixmap = m_indicatorOn;
    } else if (state == IndicatorState::Error) {
        pixmap = m_indicatorError;
    } else {
        pixmap = m_indicatorOff;
    }

    label->setPixmap(scaledIndicatorPixmap(pixmap));
}

void MainWindow::reportActionResult(const QString& action, bool success, const QString& detail, bool showFailureDialog)
{
    const QString message = QStringLiteral("%1: %2").arg(action, detail);
    logMessage(message);

    if (m_feedbackLabel) {
        m_feedbackLabel->setText(message);
        m_feedbackLabel->setStyleSheet(UiHelpers::statusStyle(success ? QStringLiteral("success")
                                                                      : QStringLiteral("error")));
    }

    if (!success && showFailureDialog) {
        QMessageBox::warning(this, textForKey(QStringLiteral("failed_suffix")).arg(action), detail);
    }
}

#pragma once

#include <QPixmap>
#include <QVector>
#include <QWidget>

#include "application/services/controllerservice.h"
#include "application/services/ioconfigurationservice.h"
#include "domain/models/iopoint.h"
#include "infrastructure/logging/filelogger.h"

class QApplication;
class QCheckBox;
class QDialog;
class QFileSystemWatcher;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSplitter;
class QSpinBox;
class QTableWidget;
class QTextEdit;
class QTimer;
class QVBoxLayout;

struct IoWidgetBinding
{
    IoPoint point;
    QLabel* indicatorLabel = nullptr;
    QCheckBox* doSwitch = nullptr;
    QLineEdit* aiDisplay = nullptr;
    QLineEdit* aoInput = nullptr;
};

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QApplication& application, ControllerService& controllerService, QWidget* parent = nullptr);
    void setLanguage(const QString& languageCode);
    void setControllerConnected(bool connected);
    void setPollIntervalMs(int value);
    int pollIntervalMs() const;
    bool refreshNow(QString* summary = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void refreshIoState();
    void handleDoSwitchToggled(bool checked);
    void handleAoEditingFinished();
    void handleConfigFileChanged(const QString& path);
    void reloadIoConfigFromFile();
    void browseConfigFile();
    void saveIoConfig();
    void addIoPoint();
    void removeSelectedIoPoint();
    void openConfigEditorDialog();
    void handlePollIntervalChanged(int value);

private:
    enum class IndicatorState
    {
        Off,
        On,
        Error
    };

    void buildUi();
    void retranslateUi();
    QString textForKey(const QString& key) const;
    int calculateIoColumns() const;
    void loadIoConfig();
    void rebuildModuleGroups();
    void populateEditorTable();
    QVector<IoPoint> collectPointsFromEditor(QString* errorMessage) const;
    void applyConfigFilePath(const QString& filePath);
    void updateWatcherPath(const QString& filePath);
    void updateIoControlsEnabled(bool connected);
    bool refreshIoStateInternal(bool reportManual, QString* summary = nullptr);
    void logMessage(const QString& message);
    QPixmap scaledIndicatorPixmap(const QPixmap& pixmap) const;
    QWidget* createPointRow(const IoPoint& point);
    void updateIndicator(QLabel* label, int value);
    void updateIndicatorError(QLabel* label);
    void updateIndicator(QLabel* label, IndicatorState state);
    void reportActionResult(const QString& action, bool success, const QString& detail, bool showFailureDialog = true);

    QApplication& m_application;
    ControllerService& m_controllerService;
    QVector<IoPoint> m_points;
    QVector<IoWidgetBinding> m_bindings;
    IoConfigurationService m_ioConfigurationService;
    FileLogger m_logger;
    QString m_languageCode = QStringLiteral("zh-CN");
    QString m_configFilePath;

    QPixmap m_indicatorOff;
    QPixmap m_indicatorOn;
    QPixmap m_indicatorError;

    QFileSystemWatcher* m_configWatcher = nullptr;
    QLabel* m_feedbackLabel = nullptr;
    QLabel* m_ioOverviewLabel = nullptr;
    QLabel* m_pollLabel = nullptr;
    QPushButton* m_settingsButton = nullptr;
    QSpinBox* m_pollIntervalEdit = nullptr;
    QScrollArea* m_ioScrollArea = nullptr;
    QWidget* m_ioContainer = nullptr;
    QVBoxLayout* m_ioContainerLayout = nullptr;
    QDialog* m_configEditorDialog = nullptr;
    QWidget* m_editorPanel = nullptr;
    QLabel* m_configEditorLabel = nullptr;
    QLabel* m_configFileLabel = nullptr;
    QLineEdit* m_configFileEdit = nullptr;
    QPushButton* m_browseConfigButton = nullptr;
    QPushButton* m_addIoButton = nullptr;
    QPushButton* m_removeIoButton = nullptr;
    QPushButton* m_saveConfigButton = nullptr;
    QTableWidget* m_configTable = nullptr;
    QLabel* m_runtimeLogLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    QTimer* m_pollTimer = nullptr;
    int m_currentIoColumns = 0;
    QString m_lastAutoRefreshSummary;
};

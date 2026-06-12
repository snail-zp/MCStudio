#pragma once

#include <QString>

#include "application/services/commoninterfaceexecutionservice.h"
#include "application/services/commoninterfaceservice.h"
#include "application/services/commoninterfaceworkflowservice.h"
#include "application/services/controllerservice.h"
#include "presentation/loggedpagewidget.h"

class QFileSystemWatcher;
class QDialog;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QShowEvent;
class QTabWidget;
class QTableWidget;
class QTextEdit;
class QVBoxLayout;

class CommonInterfacePage : public LoggedPageWidget
{
    Q_OBJECT

public:
    explicit CommonInterfacePage(ControllerService& controllerService, QWidget* parent = nullptr);
    void setLanguage(const QString& languageCode);

private slots:
    void reloadConfig();
    void openConfigEditorDialog();
    void browseConfigFile();
    void addCommandRow();
    void removeSelectedCommandRow();
    void saveConfig();
    void handleConfigFileChanged(const QString& path);
    void handleConfigDirectoryChanged(const QString& path);

private:
    void showEvent(QShowEvent* event) override;
    void buildUi();
    void buildConfigEditorDialog();
    void retranslateUi();
    void rebuildCommandGroups();
    void populateEditorTable();
    void updateWatcher();
    void showStatusMessage(const QString& text, bool isError);
    void logMessage(const QString& message);
    void flushDeferredReload();
    bool persistTransferConfigState();
    void handleCommandExecution(const CommonInterfaceCommand& command);
    void handleTransferStationExecution(const CommonInterfaceMaterialTransferStation& station);
    void handleTransferToRobotExecution();
    bool executeTransferToRobot(QString* errorMessage);
    bool executeTransferStation(const CommonInterfaceMaterialTransferStation& station, QString* errorMessage);
    CommonInterfaceCommandList collectCommandsFromEditor(QString* errorMessage) const;
    CommonInterfaceMaterialTransferConfig collectTransferConfigFromEditor(QString* errorMessage) const;
    QString textForKey(const QString& key) const;
    QString configFilePath() const;

    ControllerService& m_controllerService;
    CommonInterfaceExecutionService m_executionService;
    CommonInterfaceService m_service;
    CommonInterfaceWorkflowService m_workflowService;
    QString m_languageCode = QStringLiteral("zh-CN");
    QString m_configFilePath;
    CommonInterfaceConfig m_config;

    QPushButton* m_settingsButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QLabel* m_runtimeLogLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    QFileSystemWatcher* m_configWatcher = nullptr;
    bool m_ignoreConfigWatcherChanges = false;
    bool m_actionInProgress = false;
    bool m_reloadDeferred = false;
    bool m_initialConfigLoaded = false;

    QDialog* m_configEditorDialog = nullptr;
    QLabel* m_editorTitleLabel = nullptr;
    QLabel* m_editorConfigFileLabel = nullptr;
    QLineEdit* m_editorConfigFileEdit = nullptr;
    QPushButton* m_editorBrowseButton = nullptr;
    QPushButton* m_editorAddButton = nullptr;
    QPushButton* m_editorRemoveButton = nullptr;
    QPushButton* m_editorSaveButton = nullptr;
    QLabel* m_editorRobotNameLabel = nullptr;
    QLineEdit* m_editorRobotNameEdit = nullptr;
    QLabel* m_editorMaterialLocationLabel = nullptr;
    QLineEdit* m_editorMaterialLocationEdit = nullptr;
    QTabWidget* m_editorTabWidget = nullptr;
    QTableWidget* m_editorTable = nullptr;
    QTableWidget* m_editorTransferTable = nullptr;
};

#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QVariant>

#include "application/services/controllerservice.h"
#include "application/services/axisperformanceexecutionservice.h"
#include "application/services/axisperformanceservice.h"
#include "application/services/axisperformanceworkflowservice.h"
#include "domain/models/axisperformanceresult.h"
#include "presentation/loggedpagewidget.h"

class QFileSystemWatcher;
class QComboBox;
class QDialog;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QTextEdit;

class AxisPerformancePage : public LoggedPageWidget
{
    Q_OBJECT

public:
    explicit AxisPerformancePage(ControllerService& controllerService, QWidget* parent = nullptr);
    void setLanguage(const QString& languageCode);

private slots:
    void reloadConfig();
    void openConfigEditorDialog();
    void browseConfigFile();
    void saveConfig();
    void handleConfigFileChanged(const QString& path);

    void handleEditorProfileChanged(int index);
    void handleProfileNameEdited(const QString& text);
    void handleProfileDescriptionEdited(const QString& text);
    void addProfile();
    void removeProfile();

    void handleAxisSelectionChanged();
    void handleAxisNameEdited(const QString& text);
    void handleAxisNumberEdited(int value);
    void addAxis();
    void removeAxis();

    void handleTestSelectionChanged();
    void addTestItem();
    void removeTestItem();
    void handleParameterSetChanged(int index);
    void addParameterSet();
    void removeParameterSet();

    void handleParameterItemChanged(QTableWidgetItem* item);
    void addParameter();
    void removeParameter();
    void importResultFile();
    void exportReport();
    void handleExecutionAxisChanged(int index);
    void handleExecutionSelectionChanged();
    void handleResultAxisChanged(int index);
    void handleResultTestChanged(int index);
    void openTestScript();
    void runTestItem();

private:
    void buildUi();
    void buildConfigEditorDialog();
    void retranslateUi();
    void updateWatcher();
    void showStatusMessage(const QString& text, bool isError);
    void logMessage(const QString& message);

    void loadEditorFromProfiles();
    void refreshEditor();
    void refreshEditorProfileSection();
    void refreshAxisList();
    void refreshAxisDetail();
    void refreshTestTable();
    void refreshParameterTable();
    void refreshParameterSetSection();
    void applyTestTemplate(AxisPerformanceTestItem* testItem, const QString& testKey);
    QString displayNameForTestKey(const QString& testKey) const;
    QString parameterDisplayName(const AxisPerformanceParameter& parameter) const;
    QString parameterSetDisplayName(const AxisPerformanceParameterSet& parameterSet, int index) const;
    bool axisHasTestKey(const AxisPerformanceAxis& axis, const QString& testKey, int ignoreIndex = -1) const;
    QVector<AxisPerformanceTestItem> defaultTestTemplates() const;
    bool supportsParameterSets(const AxisPerformanceTestItem& testItem) const;
    QVector<AxisPerformanceParameter>* currentEditorParameters();
    const QVector<AxisPerformanceParameter>* currentEditorParameters() const;
    int currentEditorParameterSetIndex() const;
    bool loadResultDocument(const QString& filePath, QString* errorMessage);
    AxisPerformanceImportedDocument parseResultDocument(const QByteArray& data, QString* errorMessage) const;
    AxisPerformanceImportedTestResult parseImportedTest(const QVariantMap& testMap) const;
    void computeMetrics(AxisPerformanceImportedTestResult* testResult, const AxisPerformanceAxis* configuredAxis) const;
    const AxisPerformanceAxis* findConfiguredAxis(const AxisPerformanceImportedAxisResult& axisResult) const;
    const AxisPerformanceTestItem* findConfiguredTest(const AxisPerformanceAxis& axis, const QString& testKey) const;
    double parameterValue(const AxisPerformanceTestItem* testItem, const QString& key, int parameterSetIndex = 0, double fallback = 0.0) const;
    QString scriptPathForTestKey(const QString& testKey) const;
    QString labelForTestKey(const QString& testKey) const;
    void refreshExecutionAxisSelector();
    void refreshExecutionTable();
    void setReportPanelVisible(bool visible);
    void populateResultSelectors();
    void populateResultTestSelector(int axisNumber, const QString& preferredTestKey = QString());
    void refreshResultView();
    QString buildReportPreviewText(const AxisPerformanceImportedAxisResult& axis,
                                   const AxisPerformanceImportedTestResult& test) const;
    QString buildReportHtml() const;
    QString buildResultGuideHtml() const;
    QString verdictForThreshold(bool passed) const;
    QJsonObject serializeResultDocument(const AxisPerformanceImportedDocument& document) const;
    AxisPerformanceImportedDocument parseResultObject(const QJsonObject& object) const;
    void recomputeDocumentMetrics(AxisPerformanceImportedDocument* document) const;
    void mergeLatestResultDocument(const AxisPerformanceImportedDocument& document);
    void selectResultViewForTest(int axisNumber, const QString& testKey);

    bool validateProfiles(const QVector<AxisPerformanceProfile>& profiles, QString* errorMessage) const;
    AxisPerformanceProfile* currentEditorProfile();
    const AxisPerformanceProfile* currentEditorProfile() const;
    AxisPerformanceAxis* currentEditorAxis();
    const AxisPerformanceAxis* currentEditorAxis() const;
    AxisPerformanceTestItem* currentEditorTestItem();
    const AxisPerformanceTestItem* currentEditorTestItem() const;
    int currentEditorProfileIndex() const;
    int currentEditorAxisIndex() const;
    int currentEditorTestIndex() const;

    QString textForKey(const QString& key) const;
    QString configFilePath() const;

    QString m_languageCode = QStringLiteral("zh-CN");
    ControllerService& m_controllerService;
    AxisPerformanceService m_service;
    AxisPerformanceExecutionService m_executionService;
    AxisPerformanceWorkflowService m_workflowService;
    QVector<AxisPerformanceProfile> m_profiles;
    QVector<AxisPerformanceProfile> m_editorProfiles;
    bool m_isRefreshingEditor = false;

    QPushButton* m_editConfigButton = nullptr;
    QPushButton* m_importResultsButton = nullptr;
    QPushButton* m_exportReportButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QComboBox* m_executionAxisCombo = nullptr;
    QComboBox* m_executionTestCombo = nullptr;
    QPushButton* m_runSelectedTestButton = nullptr;
    QPushButton* m_openSelectedScriptButton = nullptr;
    QLabel* m_resultSummaryLabel = nullptr;
    QComboBox* m_resultAxisCombo = nullptr;
    QComboBox* m_resultTestCombo = nullptr;
    QTableWidget* m_resultMetricTable = nullptr;
    QFrame* m_analysisCard = nullptr;
    QWidget* m_chartWidget = nullptr;
    QTextEdit* m_reportPreview = nullptr;
    QLabel* m_runtimeLogLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    QFileSystemWatcher* m_configWatcher = nullptr;
    AxisPerformanceImportedDocument m_resultDocument;

    QDialog* m_editorDialog = nullptr;
    QLabel* m_editorTitleLabel = nullptr;
    QLabel* m_editorPathLabel = nullptr;
    QPushButton* m_editorBrowseButton = nullptr;
    QPushButton* m_editorSaveButton = nullptr;

    QComboBox* m_editorProfileCombo = nullptr;
    QPushButton* m_addProfileButton = nullptr;
    QPushButton* m_removeProfileButton = nullptr;
    QLineEdit* m_profileNameEdit = nullptr;
    QLineEdit* m_profileDescriptionEdit = nullptr;

    QListWidget* m_axisListWidget = nullptr;
    QPushButton* m_addAxisButton = nullptr;
    QPushButton* m_removeAxisButton = nullptr;
    QLineEdit* m_axisNameEdit = nullptr;
    QLineEdit* m_axisNumberEdit = nullptr;

    QComboBox* m_testTemplateCombo = nullptr;
    QTableWidget* m_testTable = nullptr;
    QPushButton* m_addTestButton = nullptr;
    QPushButton* m_removeTestButton = nullptr;

    QComboBox* m_parameterSetCombo = nullptr;
    QPushButton* m_addParameterSetButton = nullptr;
    QPushButton* m_removeParameterSetButton = nullptr;
    QTableWidget* m_parameterTable = nullptr;
    QPushButton* m_addParameterButton = nullptr;
    QPushButton* m_removeParameterButton = nullptr;
};

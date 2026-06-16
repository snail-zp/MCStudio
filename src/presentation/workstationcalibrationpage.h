#pragma once

#include <QMap>
#include <QSet>
#include <QString>

#include "application/services/controllerservice.h"
#include "application/services/workstationcalibrationexecutionservice.h"
#include "application/services/workstationcalibrationservice.h"
#include "application/services/workstationcalibrationworkflowservice.h"
#include "presentation/loggedpagewidget.h"

class QComboBox;
class QDialog;
class QDoubleSpinBox;
class QFileSystemWatcher;
class QLabel;
class QLayout;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QTableWidget;
class QTabWidget;
class QTextEdit;
class QTimer;
class QHBoxLayout;
class QVBoxLayout;
class QShowEvent;
class QHideEvent;
class QWidget;

class WorkstationCalibrationPage : public LoggedPageWidget
{
    Q_OBJECT

public:
    explicit WorkstationCalibrationPage(ControllerService& controllerService, QWidget* parent = nullptr);
    void setLanguage(const QString& languageCode);

private slots:
    void reloadConfig();
    void handleWorkstationChanged(int index);
    void openConfigEditorDialog();
    void browseConfigFile();
    void addAxisRow();
    void removeSelectedAxisRow();
    void saveCalibrationConfig();
    void handleConfigFileChanged(const QString& path);
    void refreshAxisPositions();

private:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    struct AxisSelection
    {
        QString workstationName;
        QString moduleKey;
        QString axisName;
    };

    void buildUi();
    void buildConfigEditorDialog();
    void retranslateUi();
    void refreshHeaderControls();
    void updatePositionRefreshState();
    void rebuildCalibrationView();
    void populateEditorTable();
    void updateWatcher();
    void clearContentLayout();
    void showStatusMessage(const QString& text, bool isError);
    void logMessage(const QString& message);
    void showFailureDialog(const QString& title, const QString& message);
    void setCalibrationInteractionEnabled(bool enabled);
    bool collectCalibrationsFromEditor(QVector<WorkstationCalibration>* calibrations, QString* errorMessage) const;
    QString textForKey(const QString& key) const;
    QString configFilePath() const;
    QString axisKey(const QString& workstationName, const QString& moduleKey, const CalibrationAxis& axis) const;
    const WorkstationCalibration* currentCalibration() const;
    void moveAxisByStep(const QString& workstationName, const QString& moduleKey, const QString& axisName, double direction);
    bool moveAxisToTarget(const QString& workstationName, const QString& axisName, double targetPosition);
    bool moveToStartPosition(const QString& workstationName);
    void saveSelectedMoveSequencePosition(const QString& workstationName);
    void saveMoveSequencePositionsWithConfirmation(const QString& workstationName);
    void captureStartAxisPositions(const WorkstationCalibration& calibration);
    void refreshMoveSequenceStepButtons(const QString& workstationName);
    bool canExecuteMoveSequenceStep(const QString& workstationName, int targetIndex) const;
    bool resolveReverseMoveTarget(const QString& workstationName,
                                  int currentIndex,
                                  QString* axisName,
                                  double* reverseTarget,
                                  QString* errorMessage) const;
    void updateMoveSequenceSelectionAfterMotion(const QString& workstationName, int currentIndex, int targetIndex);
    void handleMoveSequenceStepClick(const QString& workstationName,
                                     const QString& axisName,
                                     double targetPosition,
                                     int stepIndex);
    void bindAxisConfigEditors(const QString& workstationName,
                               const QVector<AxisSelection>& axisSelections,
                               QComboBox* axisCombo,
                               QDoubleSpinBox* speedSpin,
                               QDoubleSpinBox* stepSpin,
                               QLabel* unitLabel,
                               QLabel* moduleLabel = nullptr);
    QWidget* createMoveSequenceCard(const WorkstationCalibration& calibration);
    QWidget* createModuleWidget(const CalibrationModule& module, bool includeAxisConfig = true);
    QWidget* createCalibrationPositionBar(const WorkstationCalibration& calibration);
    QWidget* createModuleMotionCard(const WorkstationCalibration& calibration, const CalibrationModule& module);
    QWidget* createCombinedAxisConfigCard(const WorkstationCalibration& calibration, const CalibrationModule& leftModule, const CalibrationModule& rightModule);
    QWidget* createModuleAxisConfigCard(const WorkstationCalibration& calibration, const CalibrationModule& module);
    QWidget* createModulePositionCard(const CalibrationModule& module);
    QWidget* createWorkstationSwitchWidget(QWidget* parent);
    QPushButton* createSettingsLaunchButton(QWidget* parent) const;
    QDoubleSpinBox* createAxisSpinBox(double value, int decimals, double minimum, const QString& suffix = QString()) const;
    QPushButton* createJogButton(const QString& text) const;

    ControllerService& m_controllerService;
    WorkstationCalibrationExecutionService m_executionService;
    WorkstationCalibrationService m_service;
    WorkstationCalibrationWorkflowService m_workflowService;
    QVector<WorkstationCalibration> m_calibrations;
    QString m_languageCode = QStringLiteral("zh-CN");
    QString m_configFilePath;
    QString m_selectedWorkstationName;

    QLabel* m_workstationLabel = nullptr;
    QLabel* m_headerStepLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_runtimeLogLabel = nullptr;
    QWidget* m_workstationSwitchContainer = nullptr;
    QComboBox* m_workstationCombo = nullptr;
    QDoubleSpinBox* m_headerStepSpin = nullptr;
    QPushButton* m_headerSettingsButton = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_positionStatusBar = nullptr;
    QHBoxLayout* m_positionStatusLayout = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QTextEdit* m_logView = nullptr;
    QFileSystemWatcher* m_configWatcher = nullptr;
    QTimer* m_positionRefreshTimer = nullptr;
    QMap<QString, QLineEdit*> m_axisPositionDisplays;
    QMap<QString, int> m_selectedMoveStepByWorkstation;
    QMap<QString, QSet<int>> m_reachedMoveStepsByWorkstation;
    QStringList m_startPositionReachedWorkstations;
    QMap<QString, QMap<QString, double>> m_startAxisPositionsByWorkstation;

    QDialog* m_configEditorDialog = nullptr;
    QLabel* m_editorTitleLabel = nullptr;
    QLabel* m_editorConfigFileLabel = nullptr;
    QLineEdit* m_editorConfigFileEdit = nullptr;
    QPushButton* m_editorReloadButton = nullptr;
    QPushButton* m_editorBrowseButton = nullptr;
    QPushButton* m_editorAddButton = nullptr;
    QPushButton* m_editorRemoveButton = nullptr;
    QPushButton* m_editorSaveButton = nullptr;
    QTabWidget* m_editorTabWidget = nullptr;
    QLabel* m_editorWorkstationSectionLabel = nullptr;
    QLabel* m_editorMoveSectionLabel = nullptr;
    QLabel* m_editorOperationSectionLabel = nullptr;
    QTableWidget* m_editorTable = nullptr;
    QTableWidget* m_editorWorkstationTable = nullptr;
    QTableWidget* m_editorMoveTable = nullptr;
    QTableWidget* m_editorOperationTable = nullptr;
    QMap<QString, QPushButton*> m_moveStepButtons;
    QMap<QString, QPushButton*> m_startPositionButtons;
    QMap<QString, QDoubleSpinBox*> m_moveSequenceUnifiedStepSpins;
    QMap<QString, QDoubleSpinBox*> m_moveSequenceJogLowSpins;
    QMap<QString, QDoubleSpinBox*> m_moveSequenceJogHighSpins;
    QStringList m_lowJogSpeedWorkstations;
    bool m_calibrationInteractionEnabled = true;
    bool m_initialConfigLoaded = false;
};

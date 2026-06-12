#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "application/services/controllerservice.h"
#include "infrastructure/logging/filelogger.h"
#include "infrastructure/theme/thememanager.h"

class QApplication;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;
class MainWindow;
class AxisPerformancePage;
class CommonInterfacePage;
class SystemLogsPage;
class WorkstationCalibrationPage;
class QWidget;

struct PlaceholderPageBinding
{
    QLabel* titleLabel = nullptr;
    QLabel* descriptionLabel = nullptr;
    QLabel* cardTitleLabel = nullptr;
    QLabel* cardTextLabel = nullptr;
    QString titleKey;
    QString descriptionKey;
};

class StudioMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit StudioMainWindow(QApplication& application, QWidget* parent = nullptr);

private slots:
    void changePage(int index);
    void applySelectedTheme();
    void applySelectedLanguage();
    void connectController();
    void disconnectController();
    void refreshControllerState();
    void exportHardwareCode();
    void updateConnectionFieldVisibility();

private:
    enum class IndicatorState
    {
        Off,
        On,
        Error
    };

    QWidget* createPlaceholderPage(const QString& titleKey, const QString& descriptionKey);
    QWidget* createSettingsPage();
    void ensureCommonInterfacePage();
    void ensureWorkstationCalibrationPage();
    void ensureAxisPerformancePage();
    void buildUi();
    void buildLeftConnectionCard(QVBoxLayout* sideLayout);
    void initializeSettings();
    void retranslateUi();
    void updateConnectionUi(bool connected);
    void updateConnectionIndicator(IndicatorState state);
    QPixmap scaledIndicatorPixmap(const QPixmap& pixmap) const;
    QString textForKey(const QString& key) const;
    QString appConfigFilePath() const;

    QApplication& m_application;
    ControllerService m_controllerService;
    FileLogger m_logger { FileLogger::defaultLogDirectoryPath() };
    ThemeManager m_themeManager;
    QString m_languageCode;
    QPixmap m_indicatorOff;
    QPixmap m_indicatorOn;
    QPixmap m_indicatorError;
    QListWidget* m_navigationList = nullptr;
    QStackedWidget* m_pageStack = nullptr;
    QWidget* m_commonInterfaceHost = nullptr;
    QLabel* m_appTitleLabel = nullptr;
    QLabel* m_appSubtitleLabel = nullptr;
    QFrame* m_connectionCard = nullptr;
    QLabel* m_connectionCardTitleLabel = nullptr;
    QLabel* m_connectionLabel = nullptr;
    QLabel* m_connectionIndicatorLabel = nullptr;
    QLabel* m_ipLabel = nullptr;
    QLabel* m_portLabel = nullptr;
    QComboBox* m_connectionTypeCombo = nullptr;
    QLineEdit* m_ipEdit = nullptr;
    QSpinBox* m_portEdit = nullptr;
    QPushButton* m_connectButton = nullptr;
    QPushButton* m_disconnectButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_exportHardwareCodeButton = nullptr;
    CommonInterfacePage* m_commonInterfacePage = nullptr;
    MainWindow* m_ioTesterPage = nullptr;
    QWidget* m_workstationCalibrationHost = nullptr;
    WorkstationCalibrationPage* m_workstationCalibrationPage = nullptr;
    QWidget* m_axisPerformanceHost = nullptr;
    AxisPerformancePage* m_axisPerformancePage = nullptr;
    SystemLogsPage* m_systemLogsPage = nullptr;
    QComboBox* m_themeCombo = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QLabel* m_settingsTitleLabel = nullptr;
    QLabel* m_settingsDescriptionLabel = nullptr;
    QLabel* m_themeLabel = nullptr;
    QLabel* m_languageLabel = nullptr;
    QLabel* m_settingsStatusLabel = nullptr;
    QListWidgetItem* m_ioTesterItem = nullptr;
    QListWidgetItem* m_interfaceTestItem = nullptr;
    QListWidgetItem* m_stationCalibrationItem = nullptr;
    QListWidgetItem* m_axisPerformanceItem = nullptr;
    QListWidgetItem* m_systemLogsItem = nullptr;
    QListWidgetItem* m_settingsItem = nullptr;
    QVector<PlaceholderPageBinding> m_placeholderPages;
};

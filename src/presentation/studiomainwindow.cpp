#include "presentation/studiomainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "presentation/mainwindow.h"
#include "application/services/workstationcalibrationservice.h"
#include "presentation/axisperformancepage.h"
#include "presentation/commoninterfacepage.h"
#include "presentation/systemlogspage.h"
#include "presentation/workstationcalibrationpage.h"

namespace
{
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

QString zh(const wchar_t* text)
{
    return QString::fromWCharArray(text);
}

QString findImagePathFromCandidates(const QStringList& candidates)
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

QString findSettingsIconPath()
{
    return findImagePathFromCandidates(
        QStringList() << QStringLiteral("set.png")
                      << QStringLiteral("settings.png")
                      << QStringLiteral("setting.png"));
}

QString calibrationConfigPath()
{
    const QStringList candidates = {
        QDir(appBasePath()).filePath(QStringLiteral("Config/workstation_calibration_simulator.json")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("Config/workstation_calibration_simulator.json")),
        QDir::current().filePath(QStringLiteral("Config/workstation_calibration_simulator.json"))
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return candidates.first();
}

QFrame* createCalibrationSummaryCard(const QString& title, QWidget* parent)
{
    auto* card = new QFrame(parent);
    card->setStyleSheet(QStringLiteral("QFrame{border:1px solid #d7dee5;border-radius:12px;background:#ffffff;}"));

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(8);

    auto* cardTitle = new QLabel(title, card);
    cardTitle->setStyleSheet(QStringLiteral("font-size:16px;font-weight:700;color:#18324a;"));
    layout->addWidget(cardTitle);
    return card;
}

QWidget* createDeferredLoadingPage(const QString& text, QWidget* parent)
{
    auto* page = new QWidget(parent);
    page->setObjectName(QStringLiteral("deferredLoadingPlaceholder"));

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 36, 36, 36);

    auto* label = new QLabel(text, page);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet(QStringLiteral("font-size:14px;color:#5f6b76;"));

    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();
    return page;
}

void removeDeferredLoadingPlaceholder(QWidget* host)
{
    if (!host) {
        return;
    }

    if (auto* placeholder = host->findChild<QWidget*>(QStringLiteral("deferredLoadingPlaceholder"))) {
        placeholder->deleteLater();
    }
}

QWidget* createCalibrationSafeFallbackPage(const QString& languageCode, QWidget* parent)
{
    auto* page = new QWidget(parent);
    page->setObjectName(QStringLiteral("calibrationSafeFallbackPage"));

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 36, 36, 36);
    layout->setSpacing(14);

    const bool english = languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0;

    auto* titleLabel = new QLabel(english
                                      ? QStringLiteral("Station Calibration")
                                      : QStringLiteral("\u5de5\u4f4d\u6807\u5b9a"),
                                  page);
    titleLabel->setStyleSheet(QStringLiteral("font-size:24px;font-weight:700;color:#18324a;"));

    auto* infoLabel = new QLabel(
        english
            ? QStringLiteral("This page is temporarily running in safe mode to prevent crash-on-open while the calibration UI is being stabilized.")
            : QStringLiteral("\u5f53\u524d\u9875\u9762\u6682\u65f6\u4ee5\u5b89\u5168\u6a21\u5f0f\u8fd0\u884c\uff0c\u7528\u4e8e\u9632\u6b62\u6253\u5f00\u5de5\u4f4d\u6807\u5b9a\u65f6\u5d29\u6e83\uff0c\u540e\u7eed\u4f1a\u7ee7\u7eed\u7a33\u5b9a\u6807\u5b9a\u754c\u9762\u3002"),
        page);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("font-size:14px;color:#5f6b76;"));

    auto* card = new QFrame(page);
    card->setStyleSheet(QStringLiteral("QFrame{border:1px solid #d7dee5;border-radius:12px;background:#ffffff;}"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(8);

    auto* cardTitle = new QLabel(english
                                     ? QStringLiteral("Safe Mode Active")
                                     : QStringLiteral("\u5b89\u5168\u6a21\u5f0f\u5df2\u542f\u7528"),
                                 card);
    cardTitle->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#18324a;"));

    auto* cardText = new QLabel(
        english
            ? QStringLiteral("Complex jog, move-sequence, and live-position widgets are temporarily disabled in this page entry to keep the application stable.")
            : QStringLiteral("\u590d\u6742\u7684\u70b9\u52a8\u3001\u79fb\u52a8\u5e8f\u5217\u548c\u5b9e\u65f6\u4f4d\u7f6e\u63a7\u4ef6\u5df2\u5728\u8be5\u5165\u53e3\u6682\u65f6\u7981\u7528\uff0c\u4ee5\u4fdd\u8bc1\u8f6f\u4ef6\u5148\u7a33\u5b9a\u8fd0\u884c\u3002"),
        card);
    cardText->setWordWrap(true);
    cardText->setStyleSheet(QStringLiteral("font-size:13px;color:#5f6b76;"));

    cardLayout->addWidget(cardTitle);
    cardLayout->addWidget(cardText);

    WorkstationCalibrationService calibrationService;
    QString errorMessage;
    const auto calibrations = calibrationService.loadCalibrations(calibrationConfigPath(), &errorMessage);

    auto* summaryCard = createCalibrationSummaryCard(
        english ? QStringLiteral("Configuration Overview") : QStringLiteral("\u914d\u7f6e\u6982\u89c8"),
        page);
    auto* summaryLayout = qobject_cast<QVBoxLayout*>(summaryCard->layout());

    const QString summaryText = calibrations.isEmpty()
        ? (errorMessage.isEmpty()
               ? (english ? QStringLiteral("No workstation calibration configuration was found.")
                          : QStringLiteral("\u672a\u627e\u5230\u5de5\u4f4d\u6807\u5b9a\u914d\u7f6e\u3002"))
               : (english ? QStringLiteral("Configuration load failed: %1").arg(errorMessage)
                          : QStringLiteral("\u914d\u7f6e\u52a0\u8f7d\u5931\u8d25\uff1a%1").arg(errorMessage)))
        : (english
               ? QStringLiteral("%1 workstations loaded. You can review modules and move steps here while the interactive calibration controls are being stabilized.")
                     .arg(calibrations.size())
               : QStringLiteral("\u5df2\u52a0\u8f7d %1 \u4e2a\u5de5\u4f4d\u914d\u7f6e\u3002\u5f53\u524d\u53ef\u5148\u67e5\u770b\u6a21\u5757\u548c\u79fb\u52a8\u6b65\u9aa4\uff0c\u4ea4\u4e92\u6807\u5b9a\u63a7\u4ef6\u4ecd\u5728\u7a33\u5b9a\u4e2d\u3002")
                     .arg(calibrations.size()));

    auto* summaryLabel = new QLabel(summaryText, summaryCard);
    summaryLabel->setWordWrap(true);
    summaryLabel->setStyleSheet(QStringLiteral("font-size:13px;color:#5f6b76;"));
    summaryLayout->addWidget(summaryLabel);

    for (const auto& calibration : calibrations) {
        auto* workstationCard = createCalibrationSummaryCard(calibration.calibrationName, page);
        auto* workstationLayout = qobject_cast<QVBoxLayout*>(workstationCard->layout());

        auto* workstationMeta = new QLabel(
            english
                ? QStringLiteral("%1 modules | %2 move steps | unified step %3")
                      .arg(calibration.modules.size())
                      .arg(calibration.moveSequence.size())
                      .arg(calibration.unifiedStep)
                : QStringLiteral("%1 \u4e2a\u6a21\u5757 | %2 \u4e2a\u79fb\u52a8\u6b65\u9aa4 | \u7edf\u4e00\u6b65\u957f %3")
                      .arg(calibration.modules.size())
                      .arg(calibration.moveSequence.size())
                      .arg(calibration.unifiedStep),
            workstationCard);
        workstationMeta->setStyleSheet(QStringLiteral("font-size:12px;color:#5f6b76;"));
        workstationLayout->addWidget(workstationMeta);

        if (!calibration.description.trimmed().isEmpty()) {
            auto* descriptionLabel = new QLabel(calibration.description, workstationCard);
            descriptionLabel->setWordWrap(true);
            descriptionLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#5f6b76;"));
            workstationLayout->addWidget(descriptionLabel);
        }

        for (const auto& module : calibration.modules) {
            QStringList axisTexts;
            for (const auto& axis : module.axes) {
                axisTexts << QStringLiteral("%1(A%2)").arg(axis.name).arg(axis.axisNumber);
            }

            const QString operationText = module.operations.keys().join(QStringLiteral(", "));
            const QString moduleText = english
                ? QStringLiteral("%1 [%2] | Axes: %3 | Ops: %4")
                      .arg(module.name, module.key, axisTexts.isEmpty() ? QStringLiteral("-") : axisTexts.join(QStringLiteral(", ")),
                           operationText.isEmpty() ? QStringLiteral("-") : operationText)
                : QStringLiteral("%1 [%2] | \u8f74\uff1a%3 | \u64cd\u4f5c\uff1a%4")
                      .arg(module.name, module.key, axisTexts.isEmpty() ? QStringLiteral("-") : axisTexts.join(QStringLiteral(", ")),
                           operationText.isEmpty() ? QStringLiteral("-") : operationText);

            auto* moduleLabel = new QLabel(moduleText, workstationCard);
            moduleLabel->setWordWrap(true);
            moduleLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#18324a;"));
            workstationLayout->addWidget(moduleLabel);
        }

        if (!calibration.moveSequence.isEmpty()) {
            QStringList steps;
            for (int index = 0; index < calibration.moveSequence.size(); ++index) {
                const auto& step = calibration.moveSequence.at(index);
                steps << QStringLiteral("%1.%2=%3").arg(index + 1).arg(step.axisName).arg(step.targetPosition);
            }

            auto* moveStepsLabel = new QLabel(
                english
                    ? QStringLiteral("Move sequence: %1").arg(steps.join(QStringLiteral("  |  ")))
                    : QStringLiteral("\u79fb\u52a8\u5e8f\u5217\uff1a%1").arg(steps.join(QStringLiteral("  |  "))),
                workstationCard);
            moveStepsLabel->setWordWrap(true);
            moveStepsLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#5f6b76;"));
            workstationLayout->addWidget(moveStepsLabel);
        }

        summaryLayout->addWidget(workstationCard);
    }

    layout->addWidget(titleLabel);
    layout->addWidget(infoLabel);
    layout->addWidget(card);
    layout->addWidget(summaryCard);
    layout->addStretch();
    return page;
}
}

StudioMainWindow::StudioMainWindow(QApplication& application, QWidget* parent)
    : QMainWindow(parent)
    , m_application(application)
    , m_themeManager(appConfigFilePath())
    , m_languageCode(m_themeManager.loadLanguageCode())
{
    m_indicatorOff.load(findImagePathFromCandidates(QStringList() << QStringLiteral("Connect1.png")
                                                                  << QStringLiteral("connect1.png")
                                                                  << QStringLiteral("(1).png")));
    m_indicatorOn.load(findImagePathFromCandidates(QStringList() << QStringLiteral("connect2.png")
                                                                 << QStringLiteral("Connect2.png")
                                                                 << QStringLiteral("(2).png")));
    m_indicatorError.load(findImagePathFromCandidates(QStringList() << QStringLiteral("IO3.png")
                                                                    << QStringLiteral("io3.png")
                                                                    << QStringLiteral("copy-copy.png")));
    if (!m_application.windowIcon().isNull()) {
        setWindowIcon(m_application.windowIcon());
    }

    m_themeManager.applyTheme(m_application, m_themeManager.loadThemeName());
    buildUi();
    initializeSettings();
    retranslateUi();
}

void StudioMainWindow::changePage(int index)
{
    if (!m_pageStack || index < 0 || index >= m_pageStack->count()) {
        return;
    }

    if (index == 0) {
        ensureCommonInterfacePage();
    }

    if (index == 2) {
        ensureWorkstationCalibrationPage();
    }

    if (index == 3) {
        ensureAxisPerformancePage();
    }

    m_pageStack->setCurrentIndex(index);
    m_logger.write(FileLogger::Category::System, QStringLiteral("Switched to page index %1").arg(index));
}

void StudioMainWindow::ensureCommonInterfacePage()
{
    if (m_commonInterfacePage || !m_commonInterfaceHost) {
        return;
    }

    auto* hostLayout = qobject_cast<QVBoxLayout*>(m_commonInterfaceHost->layout());
    if (!hostLayout) {
        hostLayout = new QVBoxLayout(m_commonInterfaceHost);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->setSpacing(0);
    }

    removeDeferredLoadingPlaceholder(m_commonInterfaceHost);

    m_commonInterfacePage = new CommonInterfacePage(m_controllerService, m_commonInterfaceHost);
    m_commonInterfacePage->setLanguage(m_languageCode);
    hostLayout->addWidget(m_commonInterfacePage);
}

void StudioMainWindow::ensureWorkstationCalibrationPage()
{
    if (m_workstationCalibrationPage || !m_workstationCalibrationHost) {
        return;
    }

    auto* hostLayout = qobject_cast<QVBoxLayout*>(m_workstationCalibrationHost->layout());
    if (!hostLayout) {
        hostLayout = new QVBoxLayout(m_workstationCalibrationHost);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->setSpacing(0);
    }

    removeDeferredLoadingPlaceholder(m_workstationCalibrationHost);

    if (auto* fallback = m_workstationCalibrationHost->findChild<QWidget*>(QStringLiteral("calibrationSafeFallbackPage"))) {
        fallback->deleteLater();
    }

    m_workstationCalibrationPage = new WorkstationCalibrationPage(m_controllerService, m_workstationCalibrationHost);
    m_workstationCalibrationPage->setLanguage(m_languageCode);
    hostLayout->addWidget(m_workstationCalibrationPage);
}

void StudioMainWindow::ensureAxisPerformancePage()
{
    if (m_axisPerformancePage || !m_axisPerformanceHost) {
        return;
    }

    auto* hostLayout = qobject_cast<QVBoxLayout*>(m_axisPerformanceHost->layout());
    if (!hostLayout) {
        hostLayout = new QVBoxLayout(m_axisPerformanceHost);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->setSpacing(0);
    }

    removeDeferredLoadingPlaceholder(m_axisPerformanceHost);

    m_axisPerformancePage = new AxisPerformancePage(m_controllerService, m_axisPerformanceHost);
    m_axisPerformancePage->setLanguage(m_languageCode);
    hostLayout->addWidget(m_axisPerformancePage);
}

void StudioMainWindow::applySelectedTheme()
{
    if (!m_themeCombo) {
        return;
    }

    QString errorMessage;
    const QString themeName = m_themeCombo->currentData().toString();
    m_themeManager.applyTheme(m_application, themeName);
    if (!m_themeManager.saveThemeName(themeName, &errorMessage)) {
        if (m_settingsStatusLabel) {
            m_settingsStatusLabel->setText(errorMessage);
            m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#8a2c2c;"));
        }
        return;
    }

    if (m_settingsStatusLabel) {
        m_settingsStatusLabel->setText(textForKey(QStringLiteral("theme_saved")).arg(themeName));
        m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#255b25;"));
    }
}

void StudioMainWindow::applySelectedLanguage()
{
    if (!m_languageCombo) {
        return;
    }

    QString errorMessage;
    const QString languageCode = m_languageCombo->currentData().toString();
    if (!m_themeManager.saveLanguageCode(languageCode, &errorMessage)) {
        if (m_settingsStatusLabel) {
            m_settingsStatusLabel->setText(errorMessage);
            m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#8a2c2c;"));
        }
        return;
    }

    m_languageCode = languageCode;
    if (m_ioTesterPage) {
        m_ioTesterPage->setLanguage(m_languageCode);
    }
    if (m_commonInterfacePage) {
        m_commonInterfacePage->setLanguage(m_languageCode);
    }
    if (m_workstationCalibrationPage) {
        m_workstationCalibrationPage->setLanguage(m_languageCode);
    }
    if (m_axisPerformancePage) {
        m_axisPerformancePage->setLanguage(m_languageCode);
    }
    if (m_systemLogsPage) {
        m_systemLogsPage->setLanguage(m_languageCode);
    }
    retranslateUi();

    if (m_settingsStatusLabel) {
        m_settingsStatusLabel->setText(textForKey(QStringLiteral("language_saved")));
        m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#255b25;"));
    }
}

void StudioMainWindow::connectController()
{
    QString errorMessage;
    ControllerService::ConnectionType connectionType = ControllerService::ConnectionType::EthernetTcp;
    if (m_connectionTypeCombo->currentIndex() == 0) {
        connectionType = ControllerService::ConnectionType::Simulator;
    } else if (m_connectionTypeCombo->currentIndex() == 2) {
        connectionType = ControllerService::ConnectionType::EthernetUdp;
    }

    if (!m_controllerService.connect(connectionType, m_ipEdit->text().trimmed(), m_portEdit->value(), &errorMessage)) {
        updateConnectionIndicator(IndicatorState::Error);
        updateConnectionUi(false);
        m_logger.write(FileLogger::Category::System, QStringLiteral("Controller connection failed: %1").arg(errorMessage));
        return;
    }

    updateConnectionUi(true);
    updateConnectionIndicator(IndicatorState::On);
    m_logger.write(FileLogger::Category::System,
                   QStringLiteral("Controller connected via %1")
                       .arg(m_connectionTypeCombo ? m_connectionTypeCombo->currentText() : QStringLiteral("Unknown")));
    if (m_ioTesterPage) {
        m_ioTesterPage->setPollIntervalMs(m_ioTesterPage->pollIntervalMs());
        m_ioTesterPage->setControllerConnected(true);
    }
}

void StudioMainWindow::disconnectController()
{
    if (!m_controllerService.isConnected()) {
        return;
    }

    m_controllerService.disconnect();
    updateConnectionUi(false);
    updateConnectionIndicator(IndicatorState::Off);
    m_logger.write(FileLogger::Category::System, QStringLiteral("Controller disconnected"));
    if (m_ioTesterPage) {
        m_ioTesterPage->setControllerConnected(false);
    }
}

void StudioMainWindow::refreshControllerState()
{
    if (m_ioTesterPage) {
        m_ioTesterPage->refreshNow();
    }
}

void StudioMainWindow::exportHardwareCode()
{
    const QString defaultPath = QDir(appBasePath()).filePath(
        QStringLiteral("Exports/hardware_%1.prg").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    const QString exportPath = QFileDialog::getSaveFileName(
        this,
        textForKey(QStringLiteral("export_hardware")),
        defaultPath,
        QStringLiteral("Program Files (*.prg);;Text Files (*.txt);;All Files (*.*)"));
    if (exportPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_controllerService.exportHardwareCode(exportPath, &errorMessage)) {
        if (m_settingsStatusLabel) {
            m_settingsStatusLabel->setText(errorMessage);
            m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#8a2c2c;"));
        }
        m_logger.write(FileLogger::Category::System, QStringLiteral("Export hardware code failed: %1").arg(errorMessage));
        return;
    }

    if (m_settingsStatusLabel) {
        m_settingsStatusLabel->setText(QStringLiteral("Hardware code exported: %1").arg(exportPath));
        m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#255b25;"));
    }
    m_logger.write(FileLogger::Category::System, QStringLiteral("Hardware code exported: %1").arg(exportPath));
}

void StudioMainWindow::updateConnectionFieldVisibility()
{
    const bool isSimulator = (m_connectionTypeCombo && m_connectionTypeCombo->currentIndex() == 0);

    if (m_ipLabel) m_ipLabel->setVisible(!isSimulator);
    if (m_ipEdit) m_ipEdit->setVisible(!isSimulator);
    if (m_portLabel) m_portLabel->setVisible(!isSimulator);
    if (m_portEdit) m_portEdit->setVisible(!isSimulator);
}

QWidget* StudioMainWindow::createPlaceholderPage(const QString& titleKey, const QString& descriptionKey)
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 36, 36, 36);
    layout->setSpacing(12);

    auto* titleLabel = new QLabel(page);
    titleLabel->setStyleSheet(QStringLiteral("font-size:24px;font-weight:700;"));

    auto* descriptionLabel = new QLabel(page);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(QStringLiteral("font-size:14px;color:#5f6b76;"));

    auto* card = new QFrame(page);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(QStringLiteral("QFrame{border:1px solid #d7dee5;border-radius:12px;background:#ffffff;}"));

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(8);

    auto* cardTitleLabel = new QLabel(card);
    cardTitleLabel->setStyleSheet(QStringLiteral("font-size:18px;font-weight:600;"));

    auto* cardTextLabel = new QLabel(card);
    cardTextLabel->setWordWrap(true);
    cardTextLabel->setStyleSheet(QStringLiteral("font-size:13px;color:#5f6b76;"));

    cardLayout->addWidget(cardTitleLabel);
    cardLayout->addWidget(cardTextLabel);

    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);
    layout->addWidget(card);
    layout->addStretch();

    PlaceholderPageBinding binding;
    binding.titleLabel = titleLabel;
    binding.descriptionLabel = descriptionLabel;
    binding.cardTitleLabel = cardTitleLabel;
    binding.cardTextLabel = cardTextLabel;
    binding.titleKey = titleKey;
    binding.descriptionKey = descriptionKey;
    m_placeholderPages.push_back(binding);
    return page;
}

QWidget* StudioMainWindow::createSettingsPage()
{
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(36, 36, 36, 36);
    layout->setSpacing(16);

    m_settingsTitleLabel = new QLabel(page);
    m_settingsTitleLabel->setStyleSheet(QStringLiteral("font-size:24px;font-weight:700;"));

    m_settingsDescriptionLabel = new QLabel(page);
    m_settingsDescriptionLabel->setWordWrap(true);
    m_settingsDescriptionLabel->setStyleSheet(QStringLiteral("font-size:14px;color:#5f6b76;"));

    auto* card = new QFrame(page);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(QStringLiteral("QFrame{border:1px solid #d7dee5;border-radius:12px;background:#ffffff;}"));

    auto* formLayout = new QFormLayout(card);
    formLayout->setContentsMargins(20, 20, 20, 20);
    formLayout->setSpacing(12);

    m_themeCombo = new QComboBox(card);
    m_themeCombo->addItem(QStringLiteral("Light"), QStringLiteral("Light"));
    m_themeCombo->addItem(QStringLiteral("Dark"), QStringLiteral("Dark"));
    m_themeCombo->addItem(QStringLiteral("Blue"), QStringLiteral("Blue"));

    m_languageCombo = new QComboBox(card);
    m_languageCombo->addItem(zh(L"简体中文"), QStringLiteral("zh-CN"));
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en-US"));

    formLayout->addRow(QString(), m_themeCombo);
    formLayout->addRow(QString(), m_languageCombo);

    m_themeLabel = qobject_cast<QLabel*>(formLayout->labelForField(m_themeCombo));
    m_languageLabel = qobject_cast<QLabel*>(formLayout->labelForField(m_languageCombo));

    m_settingsStatusLabel = new QLabel(page);
    m_settingsStatusLabel->setWordWrap(true);

    layout->addWidget(m_settingsTitleLabel);
    layout->addWidget(m_settingsDescriptionLabel);
    layout->addWidget(card);
    layout->addWidget(m_settingsStatusLabel);
    layout->addStretch();

    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, &StudioMainWindow::applySelectedTheme);
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, &StudioMainWindow::applySelectedLanguage);
    return page;
}

void StudioMainWindow::buildUi()
{
    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* sidePanel = new QFrame(centralWidget);
    sidePanel->setFixedWidth(300);
    sidePanel->setStyleSheet(QStringLiteral("QFrame{background:#16324f;color:white;}"));

    auto* sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(18, 20, 18, 20);
    sideLayout->setSpacing(16);

    m_appTitleLabel = new QLabel(QStringLiteral("MCStudio"), sidePanel);
    m_appTitleLabel->setStyleSheet(QStringLiteral("font-size:26px;font-weight:800;color:white;"));

    m_appSubtitleLabel = new QLabel(sidePanel);
    m_appSubtitleLabel->setStyleSheet(QStringLiteral("font-size:12px;color:#b9c9d8;"));

    buildLeftConnectionCard(sideLayout);

    m_navigationList = new QListWidget(sidePanel);
    m_navigationList->setFrameShape(QFrame::NoFrame);
    m_navigationList->setSpacing(6);
    m_navigationList->setStyleSheet(
        QStringLiteral(
            "QListWidget{background:transparent;color:white;outline:none;}"
            "QListWidget::item{padding:12px 14px;border-radius:8px;}"
            "QListWidget::item:selected{background:#2f567d;font-weight:700;}"
            "QListWidget::item:hover:!selected{background:#234567;}"));

    m_interfaceTestItem = new QListWidgetItem(m_navigationList);
    m_ioTesterItem = new QListWidgetItem(m_navigationList);
    m_stationCalibrationItem = new QListWidgetItem(m_navigationList);
    m_axisPerformanceItem = new QListWidgetItem(m_navigationList);
    m_systemLogsItem = new QListWidgetItem(m_navigationList);
    m_settingsItem = new QListWidgetItem(m_navigationList);

    const QString settingsIconPath = findSettingsIconPath();
    if (!settingsIconPath.isEmpty()) {
        m_settingsItem->setIcon(QIcon(settingsIconPath));
    }

    sideLayout->insertWidget(0, m_appTitleLabel);
    sideLayout->insertWidget(1, m_appSubtitleLabel);
    sideLayout->addWidget(m_navigationList, 1);
    m_exportHardwareCodeButton = new QPushButton(sidePanel);
    m_exportHardwareCodeButton->setStyleSheet(QStringLiteral(
        "QPushButton{background:#234567;color:white;border:1px solid #3d6a95;border-radius:8px;padding:10px 12px;text-align:left;}"
        "QPushButton:hover{background:#2b5279;}"
        "QPushButton:disabled{background:#20384d;color:#7f95aa;border-color:#31597d;}"));
    sideLayout->addWidget(m_exportHardwareCodeButton);

    m_pageStack = new QStackedWidget(centralWidget);
    m_pageStack->setStyleSheet(QStringLiteral("QStackedWidget{background:#eef3f8;}"));

    m_commonInterfaceHost = new QWidget(m_pageStack);
    m_ioTesterPage = new MainWindow(m_application, m_controllerService, m_pageStack);
    m_workstationCalibrationHost = new QWidget(m_pageStack);
    m_axisPerformanceHost = new QWidget(m_pageStack);
    auto* commonInterfaceHostLayout = new QVBoxLayout(m_commonInterfaceHost);
    commonInterfaceHostLayout->setContentsMargins(0, 0, 0, 0);
    commonInterfaceHostLayout->setSpacing(0);
    commonInterfaceHostLayout->addWidget(createDeferredLoadingPage(
        m_languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("Loading common interface page...")
            : QStringLiteral("\u6b63\u5728\u52a0\u8f7d\u5e38\u7528\u63a5\u53e3\u9875\u9762..."),
        m_commonInterfaceHost));

    auto* calibrationHostLayout = new QVBoxLayout(m_workstationCalibrationHost);
    calibrationHostLayout->setContentsMargins(0, 0, 0, 0);
    calibrationHostLayout->setSpacing(0);
    calibrationHostLayout->addWidget(createDeferredLoadingPage(
        m_languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("Loading station calibration page...")
            : QStringLiteral("\u6b63\u5728\u52a0\u8f7d\u5de5\u4f4d\u6807\u5b9a\u9875\u9762..."),
        m_workstationCalibrationHost));

    auto* axisPerformanceHostLayout = new QVBoxLayout(m_axisPerformanceHost);
    axisPerformanceHostLayout->setContentsMargins(0, 0, 0, 0);
    axisPerformanceHostLayout->setSpacing(0);
    axisPerformanceHostLayout->addWidget(createDeferredLoadingPage(
        m_languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0
            ? QStringLiteral("Loading axis performance page...")
            : QStringLiteral("\u6b63\u5728\u52a0\u8f7d\u8f74\u6027\u80fd\u6d4b\u8bd5\u9875\u9762..."),
        m_axisPerformanceHost));

    m_pageStack->addWidget(m_commonInterfaceHost);
    m_pageStack->addWidget(m_ioTesterPage);
    m_pageStack->addWidget(m_workstationCalibrationHost);
    m_pageStack->addWidget(m_axisPerformanceHost);
    m_systemLogsPage = new SystemLogsPage(m_pageStack);
    m_systemLogsPage->setLanguage(m_languageCode);
    m_pageStack->addWidget(m_systemLogsPage);
    m_pageStack->addWidget(createSettingsPage());

    rootLayout->addWidget(sidePanel);
    rootLayout->addWidget(m_pageStack, 1);

    setCentralWidget(centralWidget);
    resize(1680, 940);
    setWindowTitle(QStringLiteral("MCStudio"));

    connect(m_navigationList, &QListWidget::currentRowChanged, this, &StudioMainWindow::changePage);
    connect(m_exportHardwareCodeButton, &QPushButton::clicked, this, &StudioMainWindow::exportHardwareCode);
    m_pageStack->setCurrentIndex(0);
    m_navigationList->setCurrentRow(0);

    QTimer::singleShot(0, this, [this]() {
        ensureCommonInterfacePage();
    });
}

void StudioMainWindow::buildLeftConnectionCard(QVBoxLayout* sideLayout)
{
    m_connectionCard = new QFrame(this);
    m_connectionCard->setStyleSheet(
        QStringLiteral("QFrame{background:#1f425f;border:1px solid #31597d;border-radius:12px;color:white;}"
                       "QLabel{color:white;} QLineEdit,QComboBox,QSpinBox{background:white;color:#20384d;border-radius:6px;padding:4px;}"));

    auto* layout = new QVBoxLayout(m_connectionCard);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    m_connectionCardTitleLabel = new QLabel(m_connectionCard);
    m_connectionCardTitleLabel->setStyleSheet(QStringLiteral("font-size:16px;font-weight:700;"));

    auto* connectionRow = new QHBoxLayout();
    m_connectionLabel = new QLabel(m_connectionCard);
    m_connectionTypeCombo = new QComboBox(m_connectionCard);
    m_connectionTypeCombo->addItem(QStringLiteral("Simulator"));
    m_connectionTypeCombo->addItem(QStringLiteral("Ethernet TCP"));
    m_connectionTypeCombo->addItem(QStringLiteral("Ethernet UDP"));
    m_connectionIndicatorLabel = new QLabel(m_connectionCard);
    m_connectionIndicatorLabel->setFixedSize(20, 20);
    connectionRow->addWidget(m_connectionLabel);
    connectionRow->addWidget(m_connectionTypeCombo, 1);
    connectionRow->addWidget(m_connectionIndicatorLabel);

    auto* ipRow = new QHBoxLayout();
    m_ipLabel = new QLabel(m_connectionCard);
    m_ipEdit = new QLineEdit(QStringLiteral("10.0.0.100"), m_connectionCard);
    ipRow->addWidget(m_ipLabel);
    ipRow->addWidget(m_ipEdit, 1);

    auto* portRow = new QHBoxLayout();
    m_portLabel = new QLabel(m_connectionCard);
    m_portEdit = new QSpinBox(m_connectionCard);
    m_portEdit->setRange(1, 65535);
    m_portEdit->setValue(701);
    portRow->addWidget(m_portLabel);
    portRow->addWidget(m_portEdit, 1);

    auto* buttonRow = new QHBoxLayout();
    m_connectButton = new QPushButton(m_connectionCard);
    m_disconnectButton = new QPushButton(m_connectionCard);
    m_refreshButton = new QPushButton(m_connectionCard);
    buttonRow->addWidget(m_connectButton);
    buttonRow->addWidget(m_disconnectButton);
    buttonRow->addWidget(m_refreshButton);

    layout->addWidget(m_connectionCardTitleLabel);
    layout->addLayout(connectionRow);
    layout->addLayout(ipRow);
    layout->addLayout(portRow);
    layout->addLayout(buttonRow);

    sideLayout->addWidget(m_connectionCard);

    connect(m_connectionTypeCombo, &QComboBox::currentTextChanged, this, &StudioMainWindow::updateConnectionFieldVisibility);
    connect(m_connectButton, &QPushButton::clicked, this, &StudioMainWindow::connectController);
    connect(m_disconnectButton, &QPushButton::clicked, this, &StudioMainWindow::disconnectController);
    connect(m_refreshButton, &QPushButton::clicked, this, &StudioMainWindow::refreshControllerState);
}

void StudioMainWindow::initializeSettings()
{
    if (m_themeCombo) {
        const QString themeName = m_themeManager.loadThemeName();
        const int themeIndex = m_themeCombo->findData(themeName);
        if (themeIndex >= 0) {
            QSignalBlocker blocker(m_themeCombo);
            m_themeCombo->setCurrentIndex(themeIndex);
        }
    }

    if (m_languageCombo) {
        const int languageIndex = m_languageCombo->findData(m_languageCode);
        if (languageIndex >= 0) {
            QSignalBlocker blocker(m_languageCombo);
            m_languageCombo->setCurrentIndex(languageIndex);
        }
    }

    if (m_ioTesterPage) {
        m_ioTesterPage->setLanguage(m_languageCode);
    }
    if (m_commonInterfacePage) {
        m_commonInterfacePage->setLanguage(m_languageCode);
    }
    if (m_workstationCalibrationPage) {
        m_workstationCalibrationPage->setLanguage(m_languageCode);
    }
    if (m_axisPerformancePage) {
        m_axisPerformancePage->setLanguage(m_languageCode);
    }
    if (m_systemLogsPage) {
        m_systemLogsPage->setLanguage(m_languageCode);
    }

    updateConnectionUi(false);
    updateConnectionIndicator(IndicatorState::Off);
    updateConnectionFieldVisibility();
}

void StudioMainWindow::retranslateUi()
{
    if (m_appSubtitleLabel) m_appSubtitleLabel->setText(textForKey(QStringLiteral("subtitle")));
    if (m_connectionCardTitleLabel) m_connectionCardTitleLabel->setText(textForKey(QStringLiteral("controller_connection")));
    if (m_connectionLabel) m_connectionLabel->setText(textForKey(QStringLiteral("connection")));
    if (m_ipLabel) m_ipLabel->setText(textForKey(QStringLiteral("controller_ip")));
    if (m_portLabel) m_portLabel->setText(textForKey(QStringLiteral("port")));
    if (m_connectButton) m_connectButton->setText(textForKey(QStringLiteral("connect")));
    if (m_disconnectButton) m_disconnectButton->setText(textForKey(QStringLiteral("disconnect")));
    if (m_refreshButton) m_refreshButton->setText(textForKey(QStringLiteral("refresh")));
    if (m_exportHardwareCodeButton) m_exportHardwareCodeButton->setText(textForKey(QStringLiteral("export_hardware")));

    if (m_connectionTypeCombo) {
        const int currentIndex = m_connectionTypeCombo->currentIndex();
        QSignalBlocker blocker(m_connectionTypeCombo);
        m_connectionTypeCombo->setItemText(0, textForKey(QStringLiteral("simulator")));
        m_connectionTypeCombo->setItemText(1, textForKey(QStringLiteral("ethernet_tcp")));
        m_connectionTypeCombo->setItemText(2, textForKey(QStringLiteral("ethernet_udp")));
        m_connectionTypeCombo->setCurrentIndex(currentIndex);
    }

    if (m_interfaceTestItem) m_interfaceTestItem->setText(textForKey(QStringLiteral("interface_test")));
    if (m_ioTesterItem) m_ioTesterItem->setText(textForKey(QStringLiteral("io_tester")));
    if (m_stationCalibrationItem) m_stationCalibrationItem->setText(textForKey(QStringLiteral("station_calibration")));
    if (m_axisPerformanceItem) m_axisPerformanceItem->setText(textForKey(QStringLiteral("axis_performance")));
    if (m_systemLogsItem) m_systemLogsItem->setText(textForKey(QStringLiteral("system_logs")));
    if (m_settingsItem) m_settingsItem->setText(textForKey(QStringLiteral("settings")));

    for (const PlaceholderPageBinding& binding : m_placeholderPages) {
        if (binding.titleLabel) binding.titleLabel->setText(textForKey(binding.titleKey));
        if (binding.descriptionLabel) binding.descriptionLabel->setText(textForKey(binding.descriptionKey));
        if (binding.cardTitleLabel) binding.cardTitleLabel->setText(textForKey(QStringLiteral("coming_soon")));
        if (binding.cardTextLabel) binding.cardTextLabel->setText(textForKey(QStringLiteral("placeholder_card")));
    }

    if (m_settingsTitleLabel) m_settingsTitleLabel->setText(textForKey(QStringLiteral("settings")));
    if (m_settingsDescriptionLabel) m_settingsDescriptionLabel->setText(textForKey(QStringLiteral("settings_desc")));
    if (m_themeLabel) m_themeLabel->setText(textForKey(QStringLiteral("theme")));
    if (m_languageLabel) m_languageLabel->setText(textForKey(QStringLiteral("language")));

    if (m_languageCombo) {
        const int currentIndex = m_languageCombo->currentIndex();
        QSignalBlocker blocker(m_languageCombo);
        m_languageCombo->setItemText(0, textForKey(QStringLiteral("language_zh")));
        m_languageCombo->setItemText(1, textForKey(QStringLiteral("language_en")));
        m_languageCombo->setCurrentIndex(currentIndex);
    }

    if (m_settingsStatusLabel && m_settingsStatusLabel->text().isEmpty()) {
        m_settingsStatusLabel->setText(textForKey(QStringLiteral("settings_hint")));
        m_settingsStatusLabel->setStyleSheet(QStringLiteral("color:#5f6b76;"));
    }
}

void StudioMainWindow::updateConnectionUi(bool connected)
{
    if (m_connectButton) m_connectButton->setEnabled(!connected);
    if (m_disconnectButton) m_disconnectButton->setEnabled(connected);
    if (m_refreshButton) m_refreshButton->setEnabled(connected);
    if (m_exportHardwareCodeButton) m_exportHardwareCodeButton->setEnabled(connected);
}

void StudioMainWindow::updateConnectionIndicator(IndicatorState state)
{
    if (!m_connectionIndicatorLabel) {
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

    m_connectionIndicatorLabel->setPixmap(scaledIndicatorPixmap(pixmap));
}

QPixmap StudioMainWindow::scaledIndicatorPixmap(const QPixmap& pixmap) const
{
    return pixmap.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QString StudioMainWindow::textForKey(const QString& key) const
{
    const bool english = (m_languageCode == QStringLiteral("en-US"));

    if (key == QStringLiteral("subtitle")) return english ? QStringLiteral("Motion Control Workbench") : zh(L"运动控制工作台");
    if (key == QStringLiteral("controller_connection")) return english ? QStringLiteral("Controller Connection") : zh(L"控制器连接");
    if (key == QStringLiteral("connection")) return english ? QStringLiteral("Connection") : zh(L"连接方式");
    if (key == QStringLiteral("simulator")) return english ? QStringLiteral("Simulator") : zh(L"模拟器");
    if (key == QStringLiteral("ethernet_tcp")) return english ? QStringLiteral("Ethernet TCP") : zh(L"以太网 TCP");
    if (key == QStringLiteral("ethernet_udp")) return english ? QStringLiteral("Ethernet UDP") : zh(L"以太网 UDP");
    if (key == QStringLiteral("controller_ip")) return english ? QStringLiteral("Controller IP") : zh(L"控制器 IP");
    if (key == QStringLiteral("port")) return english ? QStringLiteral("Port") : zh(L"端口");
    if (key == QStringLiteral("poll_ms")) return english ? QStringLiteral("Poll (ms)") : zh(L"轮询周期 (ms)");
    if (key == QStringLiteral("connect")) return english ? QStringLiteral("Connect") : zh(L"连接");
    if (key == QStringLiteral("disconnect")) return english ? QStringLiteral("Disconnect") : zh(L"断开");
    if (key == QStringLiteral("refresh")) return english ? QStringLiteral("Refresh") : zh(L"刷新");
    if (key == QStringLiteral("export_hardware")) return english ? QStringLiteral("Export Code") : zh(L"导出硬件代码");
    if (key == QStringLiteral("interface_test")) return english ? QStringLiteral("Common Interface") : zh(L"常用接口");
    if (key == QStringLiteral("io_tester")) return english ? QStringLiteral("IO Tester") : zh(L"IO 测试");
    if (key == QStringLiteral("station_calibration")) return english ? QStringLiteral("Station Calibration") : zh(L"工位标定");
    if (key == QStringLiteral("axis_performance")) return english ? QStringLiteral("Axis Performance") : zh(L"轴性能测试");
    if (key == QStringLiteral("system_logs")) return english ? QStringLiteral("System Logs") : zh(L"系统日志");
    if (key == QStringLiteral("settings")) return english ? QStringLiteral("Settings") : zh(L"设置");
    if (key == QStringLiteral("interface_test_desc")) return english ? QStringLiteral("Centralize the most-used controller commands, status reads, and quick interaction entry points.")
                                                                      : zh(L"集中放置控制器最常用的命令、状态读取和快捷交互入口。");
    if (key == QStringLiteral("station_calibration_desc")) return english ? QStringLiteral("Host calibration flows, parameter download, and result traceability for each station.")
                                                                           : zh(L"用于承载工位标定流程、参数下发以及标定结果追溯。");
    if (key == QStringLiteral("axis_performance_desc")) return english ? QStringLiteral("Analyze motion response, positioning accuracy, and vibration trends for each axis.")
                                                                        : zh(L"用于分析各运动轴的响应、定位精度和振动趋势。");
    if (key == QStringLiteral("system_logs_desc")) return english ? QStringLiteral("Aggregate controller logs, operator actions, and test records for troubleshooting.")
                                                                   : zh(L"用于汇总控制器日志、操作记录和测试结果，方便排障追溯。");
    if (key == QStringLiteral("coming_soon")) return english ? QStringLiteral("Coming Soon") : zh(L"即将上线");
    if (key == QStringLiteral("placeholder_card")) return english ? QStringLiteral("The page shell is ready. We can plug in the detailed workflow and device logic next.")
                                                                   : zh(L"页面骨架已经预留完成，后续可以继续接入详细流程和设备逻辑。");
    if (key == QStringLiteral("settings_desc")) return english ? QStringLiteral("Manage appearance and language for the whole studio here.")
                                                                : zh(L"在这里统一管理整个 MCStudio 的外观主题和界面语言。");
    if (key == QStringLiteral("theme")) return english ? QStringLiteral("Theme") : zh(L"主题");
    if (key == QStringLiteral("language")) return english ? QStringLiteral("Language") : zh(L"语言");
    if (key == QStringLiteral("language_zh")) return english ? QStringLiteral("Chinese") : zh(L"简体中文");
    if (key == QStringLiteral("language_en")) return english ? QStringLiteral("English") : QStringLiteral("English");
    if (key == QStringLiteral("settings_hint")) return english ? QStringLiteral("Changes are saved to Config/app_config.json immediately.")
                                                                : zh(L"修改后会立即保存到 Config/app_config.json。");
    if (key == QStringLiteral("theme_saved")) return english ? QStringLiteral("Theme saved: %1") : zh(L"主题已保存: %1");
    if (key == QStringLiteral("language_saved")) return english ? QStringLiteral("Language saved") : zh(L"语言已保存");
    return key;
}

QString StudioMainWindow::appConfigFilePath() const
{
    return QDir(appBasePath()).filePath(QStringLiteral("Config/app_config.json"));
}

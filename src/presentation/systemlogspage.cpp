#include "presentation/systemlogspage.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHideEvent>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTextCursor>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "presentation/uihelpers.h"

namespace
{
QString trText(const QString& languageCode, const QString& english, const QString& chinese)
{
    return languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0 ? english : chinese;
}
}

SystemLogsPage::SystemLogsPage(QWidget* parent)
    : QWidget(parent)
    , m_logger(FileLogger::defaultLogDirectoryPath())
{
    buildUi();
}

void SystemLogsPage::setLanguage(const QString& languageCode)
{
    m_languageCode = languageCode;
    if (m_statusLabel && m_statusLabel->text().isEmpty()) {
        m_statusLabel->setText(textForKey(QStringLiteral("ready")));
    }
    if (m_runtimeLogLabel) {
        m_runtimeLogLabel->setText(textForKey(QStringLiteral("runtime_log")));
    }
    if (m_refreshButton) {
        m_refreshButton->setText(textForKey(QStringLiteral("refresh")));
    }
    if (m_openFolderButton) {
        m_openFolderButton->setText(textForKey(QStringLiteral("open_folder")));
    }
    if (m_clearButton) {
        m_clearButton->setText(textForKey(QStringLiteral("clear_view")));
    }
    if (m_filterCombo) {
        const int currentIndex = m_filterCombo->currentIndex();
        m_filterCombo->blockSignals(true);
        m_filterCombo->clear();
        m_filterCombo->addItem(textForKey(QStringLiteral("filter_all")), QString());
        m_filterCombo->addItem(QStringLiteral("IO"), QStringLiteral("[IO]"));
        m_filterCombo->addItem(QStringLiteral("COMMON"), QStringLiteral("[COMMON]"));
        m_filterCombo->addItem(QStringLiteral("CALIBRATION"), QStringLiteral("[CALIBRATION]"));
        m_filterCombo->addItem(QStringLiteral("AXIS"), QStringLiteral("[AXIS]"));
        m_filterCombo->addItem(QStringLiteral("SYSTEM"), QStringLiteral("[SYSTEM]"));
        m_filterCombo->setCurrentIndex(qBound(0, currentIndex, m_filterCombo->count() - 1));
        m_filterCombo->blockSignals(false);
    }
    if (!m_allLines.isEmpty()) {
        applyVisibleLines(filteredLines(m_allLines));
    }
}

void SystemLogsPage::refreshLogs()
{
    m_allLines = m_logger.readAllLines(1200);
    applyVisibleLines(filteredLines(m_allLines));
}

void SystemLogsPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshLogs();
    if (m_refreshTimer && !m_refreshTimer->isActive()) {
        m_refreshTimer->start();
    }
}

void SystemLogsPage::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    if (m_refreshTimer && m_refreshTimer->isActive()) {
        m_refreshTimer->stop();
    }
}

void SystemLogsPage::openLogFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_logger.logDirectoryPath()));
}

void SystemLogsPage::clearView()
{
    m_allLines.clear();
    m_cachedLines.clear();
    if (m_logView) {
        m_logView->clear();
    }
    if (m_statusLabel) {
        m_statusLabel->setText(trText(m_languageCode, QStringLiteral("Log view cleared"), QStringLiteral("日志视图已清空")));
        m_statusLabel->setStyleSheet(UiHelpers::statusStyle(QStringLiteral("info")));
    }
}

void SystemLogsPage::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(36, 28, 36, 28);
    rootLayout->setSpacing(14);

    m_statusLabel = nullptr;
    m_filterCombo = nullptr;
    m_refreshButton = nullptr;
    m_openFolderButton = nullptr;
    m_clearButton = nullptr;
    m_runtimeLogLabel = nullptr;
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(240);
    m_logView->setStyleSheet(UiHelpers::logViewStyle());

    rootLayout->addWidget(m_logView, 1);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1500);

    connect(m_refreshTimer, &QTimer::timeout, this, &SystemLogsPage::refreshLogs);

    setLanguage(m_languageCode);
}

QString SystemLogsPage::textForKey(const QString& key) const
{
    if (key == QStringLiteral("ready")) {
        return trText(m_languageCode, QStringLiteral("System logs are ready"), QStringLiteral("系统日志已就绪"));
    }
    if (key == QStringLiteral("runtime_log")) {
        return trText(m_languageCode, QStringLiteral("System Log Stream"), QStringLiteral("系统日志流"));
    }
    if (key == QStringLiteral("refresh")) {
        return trText(m_languageCode, QStringLiteral("Refresh"), QStringLiteral("刷新"));
    }
    if (key == QStringLiteral("open_folder")) {
        return trText(m_languageCode, QStringLiteral("Open Folder"), QStringLiteral("打开目录"));
    }
    if (key == QStringLiteral("clear_view")) {
        return trText(m_languageCode, QStringLiteral("Clear View"), QStringLiteral("清空视图"));
    }
    if (key == QStringLiteral("filter_all")) {
        return trText(m_languageCode, QStringLiteral("All Logs"), QStringLiteral("全部日志"));
    }
    return key;
}

QStringList SystemLogsPage::filteredLines(const QStringList& lines) const
{
    if (!m_filterCombo) {
        return lines;
    }

    const QString token = m_filterCombo->currentData().toString();
    if (token.isEmpty()) {
        return lines;
    }

    QStringList result;
    for (const QString& line : lines) {
        if (line.contains(token, Qt::CaseInsensitive)) {
            result.append(line);
        }
    }
    return result;
}

void SystemLogsPage::applyVisibleLines(const QStringList& visibleLines)
{
    if (visibleLines == m_cachedLines) {
        return;
    }

    m_cachedLines = visibleLines;
    if (m_logView) {
        m_logView->setPlainText(visibleLines.join(QLatin1Char('\n')));
        QTextCursor cursor = m_logView->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logView->setTextCursor(cursor);
    }

    if (m_statusLabel) {
        const QFileInfo currentFile(m_logger.currentLogFilePath());
        const QString text = trText(
            m_languageCode,
            QStringLiteral("Loaded %1 lines from %2").arg(visibleLines.size()).arg(currentFile.fileName()),
            QStringLiteral("已加载 %1 条日志，文件：%2").arg(visibleLines.size()).arg(currentFile.fileName()));
        m_statusLabel->setText(text);
        m_statusLabel->setStyleSheet(UiHelpers::statusStyle(QStringLiteral("info")));
    }
}

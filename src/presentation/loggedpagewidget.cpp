#include "presentation/loggedpagewidget.h"

#include "presentation/uihelpers.h"

#include <QDateTime>
#include <QLabel>
#include <QTextEdit>

LoggedPageWidget::LoggedPageWidget(FileLogger::Category logCategory, QWidget* parent)
    : QWidget(parent)
    , m_logger(FileLogger::defaultLogDirectoryPath())
    , m_logCategory(logCategory)
{
}

void LoggedPageWidget::bindFeedbackWidgets(QLabel* statusLabel, QTextEdit* logView)
{
    m_statusLabel = statusLabel;
    m_logView = logView;
}

void LoggedPageWidget::setStatusMessage(const QString& text, bool isError, bool mirrorToLog)
{
    if (m_statusLabel) {
        m_statusLabel->setText(text);
        m_statusLabel->setStyleSheet(UiHelpers::statusStyle(isError ? QStringLiteral("error")
                                                                    : QStringLiteral("success")));
    }

    if (mirrorToLog) {
        appendLogMessage(text);
    }
}

void LoggedPageWidget::appendLogMessage(const QString& message)
{
    if (m_logView) {
        const QString line = QStringLiteral("[%1] %2")
                                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz")))
                                 .arg(message);
        m_logView->append(line);
    }

    m_logger.write(m_logCategory, message);
}

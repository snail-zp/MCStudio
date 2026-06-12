#pragma once

#include <QWidget>

#include "infrastructure/logging/filelogger.h"

class QLabel;
class QTextEdit;

class LoggedPageWidget : public QWidget
{
public:
    explicit LoggedPageWidget(FileLogger::Category logCategory, QWidget* parent = nullptr);

protected:
    void bindFeedbackWidgets(QLabel* statusLabel, QTextEdit* logView);
    void setStatusMessage(const QString& text, bool isError, bool mirrorToLog = false);
    void appendLogMessage(const QString& message);

private:
    QLabel* m_statusLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    FileLogger m_logger;
    FileLogger::Category m_logCategory;
};

#pragma once

#include <QWidget>

#include "infrastructure/logging/filelogger.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTextEdit;
class QTimer;

class SystemLogsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SystemLogsPage(QWidget* parent = nullptr);
    void setLanguage(const QString& languageCode);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void refreshLogs();
    void openLogFolder();
    void clearView();

private:
    void buildUi();
    QString textForKey(const QString& key) const;
    QStringList filteredLines(const QStringList& lines) const;
    void applyVisibleLines(const QStringList& visibleLines);

    QString m_languageCode = QStringLiteral("zh-CN");
    FileLogger m_logger;
    QLabel* m_statusLabel = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_openFolderButton = nullptr;
    QPushButton* m_clearButton = nullptr;
    QLabel* m_runtimeLogLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QStringList m_allLines;
    QStringList m_cachedLines;
};

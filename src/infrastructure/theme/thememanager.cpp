#include "infrastructure/theme/thememanager.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QStyle>

namespace
{
QJsonObject loadConfigObject(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    return document.object();
}

bool saveConfigObject(const QString& filePath, const QJsonObject& object, QString* errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to save app config: %1").arg(file.errorString());
        }
        return false;
    }

    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}
}

ThemeManager::ThemeManager(const QString& configFilePath)
    : m_configFilePath(configFilePath)
{
}

QStringList ThemeManager::availableThemes() const
{
    return {QStringLiteral("Light"), QStringLiteral("Dark"), QStringLiteral("Blue")};
}

QString ThemeManager::loadThemeName() const
{
    const QJsonObject root = loadConfigObject(m_configFilePath);
    return normalizeThemeName(root.value(QStringLiteral("theme")).toString());
}

bool ThemeManager::saveThemeName(const QString& themeName, QString* errorMessage) const
{
    QJsonObject root = loadConfigObject(m_configFilePath);
    root.insert(QStringLiteral("theme"), normalizeThemeName(themeName));
    if (!root.contains(QStringLiteral("language"))) {
        root.insert(QStringLiteral("language"), QStringLiteral("zh-CN"));
    }
    return saveConfigObject(m_configFilePath, root, errorMessage);
}

QStringList ThemeManager::availableLanguages() const
{
    return {QStringLiteral("zh-CN"), QStringLiteral("en-US")};
}

QString ThemeManager::loadLanguageCode() const
{
    const QJsonObject root = loadConfigObject(m_configFilePath);
    return normalizeLanguageCode(root.value(QStringLiteral("language")).toString());
}

bool ThemeManager::saveLanguageCode(const QString& languageCode, QString* errorMessage) const
{
    QJsonObject root = loadConfigObject(m_configFilePath);
    root.insert(QStringLiteral("language"), normalizeLanguageCode(languageCode));
    if (!root.contains(QStringLiteral("theme"))) {
        root.insert(QStringLiteral("theme"), QStringLiteral("Light"));
    }
    return saveConfigObject(m_configFilePath, root, errorMessage);
}

void ThemeManager::applyTheme(QApplication& application, const QString& themeName) const
{
    const QString normalized = normalizeThemeName(themeName);
    QPalette palette = application.style()->standardPalette();
    QString styleSheet;

    if (normalized == QStringLiteral("Dark")) {
        palette.setColor(QPalette::Window, QColor(30, 30, 30));
        palette.setColor(QPalette::WindowText, QColor(212, 212, 212));
        palette.setColor(QPalette::Base, QColor(37, 37, 38));
        palette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
        palette.setColor(QPalette::Text, QColor(212, 212, 212));
        palette.setColor(QPalette::Button, QColor(45, 45, 48));
        palette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
        palette.setColor(QPalette::Highlight, QColor(14, 99, 156));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        styleSheet = QStringLiteral(
            "QWidget{font-family:'Segoe UI';font-size:12px;}"
            "QMainWindow,QStackedWidget,QScrollArea{background:#1e1e1e;color:#d4d4d4;}"
            "QLabel{color:#d4d4d4;}"
            "QFrame{background:#252526;color:#d4d4d4;}"
            "QGroupBox{font-weight:700;border:1px solid #333333;border-radius:8px;margin-top:10px;padding-top:10px;background:#252526;color:#d4d4d4;}"
            "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#c5c5c5;}"
            "QPushButton{padding:7px 12px;border-radius:6px;background:#2d2d30;border:1px solid #3c3c3c;color:#cccccc;}"
            "QPushButton:hover{background:#37373d;border-color:#007acc;}"
            "QPushButton:pressed{background:#414148;border-color:#2899f5;}"
            "QLineEdit,QTextEdit,QComboBox,QSpinBox,QListWidget,QTableWidget{background:#1e1e1e;color:#d4d4d4;border:1px solid #3c3c3c;padding:4px;border-radius:4px;selection-background-color:#264f78;}"
            "QHeaderView::section{background:#2d2d30;color:#cccccc;border:1px solid #3c3c3c;padding:6px;font-weight:600;}"
            "QTableCornerButton::section{background:#2d2d30;border:1px solid #3c3c3c;}"
            "QScrollBar:vertical{background:#1e1e1e;width:12px;margin:0;}"
            "QScrollBar::handle:vertical{background:#424242;border-radius:6px;min-height:24px;}"
            "QScrollBar::handle:vertical:hover{background:#4f4f4f;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QTabWidget::pane{border:1px solid #333333;background:#252526;}"
            "QTabBar::tab{background:#2d2d30;color:#c5c5c5;padding:8px 14px;border:1px solid #333333;border-bottom:none;}"
            "QTabBar::tab:selected{background:#1e1e1e;color:#ffffff;border-top:2px solid #007acc;}"
            "QTabBar::tab:hover{background:#37373d;}");
    } else if (normalized == QStringLiteral("Blue")) {
        palette.setColor(QPalette::Window, QColor(243, 246, 249));
        palette.setColor(QPalette::WindowText, QColor(51, 51, 51));
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
        palette.setColor(QPalette::AlternateBase, QColor(233, 237, 241));
        palette.setColor(QPalette::Text, QColor(51, 51, 51));
        palette.setColor(QPalette::Button, QColor(255, 255, 255));
        palette.setColor(QPalette::ButtonText, QColor(51, 51, 51));
        palette.setColor(QPalette::Highlight, QColor(0, 122, 204));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        styleSheet = QStringLiteral(
            "QWidget{font-family:'Segoe UI';font-size:12px;}"
            "QMainWindow,QStackedWidget,QScrollArea{background:#f3f6f9;color:#333333;}"
            "QLabel{color:#333333;}"
            "QFrame{background:#ffffff;color:#333333;}"
            "QGroupBox{font-weight:700;border:1px solid #d4dbe3;border-radius:8px;margin-top:10px;padding-top:10px;background:#ffffff;color:#333333;}"
            "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#4b5563;}"
            "QPushButton{padding:7px 12px;border-radius:6px;background:#ffffff;border:1px solid #c8d1dc;color:#333333;}"
            "QPushButton:hover{border-color:#007acc;background:#f8fbff;}"
            "QPushButton:pressed{background:#e8f2fb;border-color:#007acc;}"
            "QLineEdit,QTextEdit,QComboBox,QSpinBox,QListWidget,QTableWidget{background:#ffffff;color:#333333;border:1px solid #cfd6df;padding:4px;border-radius:4px;selection-background-color:#cce6ff;}"
            "QHeaderView::section{background:#f3f6f9;color:#4b5563;border:1px solid #d4dbe3;padding:6px;font-weight:600;}"
            "QTableCornerButton::section{background:#f3f6f9;border:1px solid #d4dbe3;}"
            "QScrollBar:vertical{background:#f3f6f9;width:12px;margin:0;}"
            "QScrollBar::handle:vertical{background:#c2cad4;border-radius:6px;min-height:24px;}"
            "QScrollBar::handle:vertical:hover{background:#aeb8c4;}"
            "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            "QTabWidget::pane{border:1px solid #d4dbe3;background:#ffffff;}"
            "QTabBar::tab{background:#eceff4;color:#4b5563;padding:8px 14px;border:1px solid #d4dbe3;border-bottom:none;}"
            "QTabBar::tab:selected{background:#ffffff;color:#111827;border-top:2px solid #007acc;}"
            "QTabBar::tab:hover{background:#f8fbff;}");
    } else {
        palette.setColor(QPalette::Window, QColor(248, 248, 248));
        palette.setColor(QPalette::WindowText, QColor(51, 51, 51));
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
        palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        palette.setColor(QPalette::Text, QColor(51, 51, 51));
        palette.setColor(QPalette::Button, QColor(255, 255, 255));
        palette.setColor(QPalette::ButtonText, QColor(51, 51, 51));
        palette.setColor(QPalette::Highlight, QColor(0, 122, 204));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        styleSheet = QStringLiteral(
            "QWidget{font-family:'Segoe UI';font-size:12px;}"
            "QMainWindow,QStackedWidget,QScrollArea{background:#f8f8f8;color:#333333;}"
            "QLabel{color:#333333;}"
            "QFrame{background:#ffffff;color:#333333;}"
            "QGroupBox{font-weight:700;border:1px solid #d9d9d9;border-radius:8px;margin-top:10px;padding-top:10px;background:#ffffff;color:#333333;}"
            "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#555555;}"
            "QPushButton{padding:7px 12px;border-radius:6px;background:#ffffff;border:1px solid #cccccc;color:#333333;}"
            "QPushButton:hover{border-color:#007acc;background:#f7fbff;}"
            "QPushButton:pressed{background:#e8f2fb;border-color:#007acc;}"
            "QLineEdit,QTextEdit,QComboBox,QSpinBox,QListWidget,QTableWidget{background:#ffffff;color:#333333;border:1px solid #cccccc;padding:4px;border-radius:4px;selection-background-color:#cce6ff;}"
            "QHeaderView::section{background:#f3f3f3;color:#555555;border:1px solid #d9d9d9;padding:6px;font-weight:600;}"
            "QTableCornerButton::section{background:#f3f3f3;border:1px solid #d9d9d9;}"
            "QTabWidget::pane{border:1px solid #d9d9d9;background:#ffffff;}"
            "QTabBar::tab{background:#efefef;color:#555555;padding:8px 14px;border:1px solid #d9d9d9;border-bottom:none;}"
            "QTabBar::tab:selected{background:#ffffff;color:#111111;border-top:2px solid #007acc;}");
    }

    application.setPalette(palette);
    application.setStyleSheet(styleSheet);
}

QString ThemeManager::normalizeThemeName(const QString& themeName) const
{
    const QString trimmed = themeName.trimmed();
    if (trimmed.compare(QStringLiteral("Dark"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Dark");
    }
    if (trimmed.compare(QStringLiteral("Blue"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Blue");
    }
    return QStringLiteral("Light");
}

QString ThemeManager::normalizeLanguageCode(const QString& languageCode) const
{
    const QString trimmed = languageCode.trimmed();
    if (trimmed.compare(QStringLiteral("en"), Qt::CaseInsensitive) == 0
        || trimmed.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("en-US");
    }
    return QStringLiteral("zh-CN");
}

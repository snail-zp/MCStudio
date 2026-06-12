#pragma once

#include <QString>
#include <QStringList>

class QApplication;

class ThemeManager
{
public:
    explicit ThemeManager(const QString& configFilePath);

    QStringList availableThemes() const;
    QString loadThemeName() const;
    bool saveThemeName(const QString& themeName, QString* errorMessage) const;
    QStringList availableLanguages() const;
    QString loadLanguageCode() const;
    bool saveLanguageCode(const QString& languageCode, QString* errorMessage) const;
    void applyTheme(QApplication& application, const QString& themeName) const;

private:
    QString normalizeThemeName(const QString& themeName) const;
    QString normalizeLanguageCode(const QString& languageCode) const;
    QString m_configFilePath;
};

#pragma once

#include <QStringList>
#include <QString>

class FileLogger
{
public:
    enum class Category
    {
        Io,
        CommonInterface,
        WorkstationCalibration,
        AxisPerformance,
        System
    };

    explicit FileLogger(const QString& logDirectoryPath);

    void write(const QString& message);
    void write(Category category, const QString& message);
    QString logDirectoryPath() const;
    QString currentLogFilePath() const;
    QStringList readAllLines(int maxLines = 2000) const;

    static QString defaultLogDirectoryPath();
    static QString categoryName(Category category);

private:
    QString m_logDirectoryPath;
};

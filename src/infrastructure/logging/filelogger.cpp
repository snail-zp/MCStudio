#include "infrastructure/logging/filelogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringConverter>
#include <QTextStream>

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
        if (QDir(candidate).exists(QStringLiteral("Config"))
            || QDir(candidate).exists(QStringLiteral("Image"))
            || QDir(candidate).exists(QStringLiteral("image"))) {
            return candidate;
        }
    }

    return QCoreApplication::applicationDirPath();
}
}

FileLogger::FileLogger(const QString& logDirectoryPath)
    : m_logDirectoryPath(logDirectoryPath)
{
}

void FileLogger::write(const QString& message)
{
    write(Category::System, message);
}

void FileLogger::write(Category category, const QString& message)
{
    QDir().mkpath(m_logDirectoryPath);
    const QString filePath = currentLogFilePath();

    QFile file(filePath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
           << " "
           << "["
           << categoryName(category)
           << "] "
           << message
           << "\n";
}

QString FileLogger::logDirectoryPath() const
{
    return m_logDirectoryPath;
}

QString FileLogger::currentLogFilePath() const
{
    return QDir(m_logDirectoryPath).filePath(
        QStringLiteral("mcstudio_%1.log").arg(QDate::currentDate().toString(QStringLiteral("yyyyMMdd"))));
}

QStringList FileLogger::readAllLines(int maxLines) const
{
    QStringList result;
    QDir dir(m_logDirectoryPath);
    if (!dir.exists()) {
        return result;
    }

    const QStringList files = dir.entryList(QStringList() << QStringLiteral("mcstudio_*.log"),
                                            QDir::Files,
                                            QDir::Time | QDir::Reversed);
    for (const QString& fileName : files) {
        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        while (!stream.atEnd()) {
            result.append(stream.readLine());
        }
    }

    if (maxLines > 0 && result.size() > maxLines) {
        result = result.mid(result.size() - maxLines);
    }
    return result;
}

QString FileLogger::defaultLogDirectoryPath()
{
    return QDir(appBasePath()).filePath(QStringLiteral("Logs"));
}

QString FileLogger::categoryName(Category category)
{
    switch (category) {
    case Category::Io:
        return QStringLiteral("IO");
    case Category::CommonInterface:
        return QStringLiteral("COMMON");
    case Category::WorkstationCalibration:
        return QStringLiteral("CALIBRATION");
    case Category::AxisPerformance:
        return QStringLiteral("AXIS");
    case Category::System:
    default:
        return QStringLiteral("SYSTEM");
    }
}

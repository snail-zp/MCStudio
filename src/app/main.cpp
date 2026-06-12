#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QByteArray>
#include <QFileInfo>

#include "presentation/studiomainwindow.h"

namespace
{
QString resolveAppIconPath()
{
    const QStringList baseDirs = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath(),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral(".."))
    };

    for (const QString& baseDir : baseDirs) {
        const QDir dir(baseDir);
        const QStringList candidates = {
            dir.filePath(QStringLiteral("Image/mcstudio_app_icon.png")),
            dir.filePath(QStringLiteral("image/mcstudio_app_icon.png")),
            dir.filePath(QStringLiteral("Image/mcstudio_app_icon.ico")),
            dir.filePath(QStringLiteral("image/mcstudio_app_icon.ico"))
        };

        for (const QString& candidate : candidates) {
            if (QFileInfo::exists(candidate)) {
                return candidate;
            }
        }
    }

    return QString();
}
}

int main(int argc, char* argv[])
{
    qputenv("QT_STYLE_OVERRIDE", QByteArray("fusion"));
    QApplication app(argc, argv);
    app.setStyle(QStringLiteral("Fusion"));
    const QString appIconPath = resolveAppIconPath();
    if (!appIconPath.isEmpty()) {
        app.setWindowIcon(QIcon(appIconPath));
    }
    StudioMainWindow window(app);
    window.show();
    return app.exec();
}

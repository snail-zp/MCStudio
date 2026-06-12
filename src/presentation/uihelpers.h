#pragma once

#include <QString>

namespace UiHelpers
{
inline QString pageTitleStyle()
{
    return QStringLiteral("font-size:24px;font-weight:700;color:#2c2c2c;");
}

inline QString pageDescriptionStyle()
{
    return QStringLiteral("font-size:14px;line-height:1.4;color:#616161;");
}

inline QString secondaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton{padding:8px 14px;border-radius:6px;background:#ffffff;border:1px solid #c8d1dc;color:#333333;font-weight:600;}"
        "QPushButton:hover{background:#f7fbff;border-color:#007acc;}"
        "QPushButton:pressed{background:#e8f2fb;border-color:#007acc;}"
        "QPushButton:disabled{background:#f3f4f6;color:#9aa3ad;border-color:#d5d9de;}");
}

inline QString primaryButtonStyle()
{
    return QStringLiteral(
        "QPushButton{padding:9px 16px;border-radius:6px;background:#0e639c;border:1px solid #1177bb;color:#ffffff;font-weight:700;}"
        "QPushButton:hover{background:#1177bb;border-color:#2899f5;}"
        "QPushButton:pressed{background:#0b5787;border-color:#0e639c;}"
        "QPushButton:disabled{background:#3b4d5c;color:#94a3ad;border-color:#3b4d5c;}");
}

inline QString cardStyle()
{
    return QStringLiteral("QFrame{background:#ffffff;border:1px solid #d4dbe3;border-radius:10px;}");
}

inline QString groupBoxStyle()
{
    return QStringLiteral(
        "QGroupBox{font-size:16px;font-weight:700;color:#2c2c2c;border:1px solid #d4dbe3;border-radius:10px;margin-top:10px;padding-top:10px;background:#ffffff;}"
        "QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 6px;color:#4b5563;}");
}

inline QString sectionLabelStyle()
{
    return QStringLiteral("font-size:13px;font-weight:700;color:#4b5563;");
}

inline QString logViewStyle()
{
    return QStringLiteral(
        "QTextEdit{background:#ffffff;border:1px solid #d4dbe3;border-radius:8px;padding:8px;color:#333333;font-family:'Consolas';font-size:12px;selection-background-color:#cce6ff;}");
}

inline QString statusStyle(const QString& mode)
{
    if (mode == QStringLiteral("error")) {
        return QStringLiteral("padding:9px 12px;border:1px solid #f1b8b8;border-radius:8px;background:#fff5f5;color:#b42318;font-weight:600;");
    }
    if (mode == QStringLiteral("success")) {
        return QStringLiteral("padding:9px 12px;border:1px solid #b7dfbe;border-radius:8px;background:#f0fbf2;color:#166534;font-weight:600;");
    }
    return QStringLiteral("padding:9px 12px;border:1px solid #d4dbe3;border-radius:8px;background:#f8fbff;color:#374151;font-weight:600;");
}
}

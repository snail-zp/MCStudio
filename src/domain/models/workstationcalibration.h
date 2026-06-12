#pragma once

#include <QMap>
#include <QString>
#include <QVector>

struct CalibrationAxis
{
    QString name;
    int axisNumber = 0;
    double defaultSpeed = 10.0;
    double defaultStep = 1.0;
    QString unit = QStringLiteral("mm");
};

struct CalibrationCommandAction
{
    QString command;
    QString doneVariable;
    int doneBuffer = -1;
    int doneValue = 1;
};

struct CalibrationModule
{
    QString key;
    QString name;
    QString description;
    QVector<CalibrationAxis> axes;
    QMap<QString, CalibrationCommandAction> operations;
};

struct CalibrationMoveStep
{
    QString axisName;
    double targetPosition = 0.0;
    QString arrivalSensorVariable;
};

struct WorkstationCalibration
{
    QString calibrationName;
    QString description;
    QVector<CalibrationModule> modules;
    QVector<CalibrationMoveStep> moveSequence;
    QString startPositionCommand;
    QString startPositionDoneVariable;
    int startPositionDoneBuffer = -1;
    int startPositionDoneValue = 1;
    double unifiedStep = 1.0;
    double jogSpeedLow = 10.0;
    double jogSpeedHigh = 50.0;
};

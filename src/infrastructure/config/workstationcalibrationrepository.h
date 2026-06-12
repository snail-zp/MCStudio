#pragma once

#include <QString>
#include <QVector>

#include "domain/models/workstationcalibration.h"

class WorkstationCalibrationRepository
{
public:
    QVector<WorkstationCalibration> load(const QString& filePath, QString* errorMessage) const;
    bool save(const QString& filePath, const QVector<WorkstationCalibration>& calibrations, QString* errorMessage) const;
};

#pragma once

#include <QString>
#include <QVector>

#include "domain/models/workstationcalibration.h"
#include "infrastructure/config/workstationcalibrationrepository.h"

class WorkstationCalibrationService
{
public:
    QVector<WorkstationCalibration> loadCalibrations(const QString& filePath, QString* errorMessage) const;
    bool saveCalibrations(const QString& filePath, const QVector<WorkstationCalibration>& calibrations, QString* errorMessage) const;

private:
    WorkstationCalibrationRepository m_repository;
};

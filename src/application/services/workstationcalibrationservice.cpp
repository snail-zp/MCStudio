#include "application/services/workstationcalibrationservice.h"

QVector<WorkstationCalibration> WorkstationCalibrationService::loadCalibrations(const QString& filePath, QString* errorMessage) const
{
    return m_repository.load(filePath, errorMessage);
}

bool WorkstationCalibrationService::saveCalibrations(const QString& filePath,
                                                     const QVector<WorkstationCalibration>& calibrations,
                                                     QString* errorMessage) const
{
    return m_repository.save(filePath, calibrations, errorMessage);
}

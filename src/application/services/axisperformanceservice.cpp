#include "application/services/axisperformanceservice.h"

QVector<AxisPerformanceProfile> AxisPerformanceService::loadProfiles(const QString& filePath, QString* errorMessage) const
{
    return m_repository.load(filePath, errorMessage);
}

bool AxisPerformanceService::saveProfiles(const QString& filePath,
                                          const QVector<AxisPerformanceProfile>& profiles,
                                          QString* errorMessage) const
{
    return m_repository.save(filePath, profiles, errorMessage);
}

#pragma once

#include <QString>
#include <QVector>

#include "domain/models/axisperformancetest.h"
#include "infrastructure/config/axisperformancerepository.h"

class AxisPerformanceService
{
public:
    QVector<AxisPerformanceProfile> loadProfiles(const QString& filePath, QString* errorMessage) const;
    bool saveProfiles(const QString& filePath, const QVector<AxisPerformanceProfile>& profiles, QString* errorMessage) const;

private:
    AxisPerformanceRepository m_repository;
};

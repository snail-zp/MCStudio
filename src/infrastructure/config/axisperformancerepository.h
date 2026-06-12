#pragma once

#include <QString>
#include <QVector>

#include "domain/models/axisperformancetest.h"

class AxisPerformanceRepository
{
public:
    QVector<AxisPerformanceProfile> load(const QString& filePath, QString* errorMessage) const;
    bool save(const QString& filePath, const QVector<AxisPerformanceProfile>& profiles, QString* errorMessage) const;
};

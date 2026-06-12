#pragma once

#include <QString>
#include <QVector>

#include "domain/models/iopoint.h"
#include "infrastructure/config/ioconfigrepository.h"

class IoConfigurationService
{
public:
    QVector<IoPoint> loadPoints(const QString& filePath, QString* errorMessage) const;
    bool savePoints(const QString& filePath, const QVector<IoPoint>& points, QString* errorMessage) const;

private:
    IoConfigRepository m_repository;
};

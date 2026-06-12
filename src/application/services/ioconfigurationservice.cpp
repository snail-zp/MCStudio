#include "application/services/ioconfigurationservice.h"

QVector<IoPoint> IoConfigurationService::loadPoints(const QString& filePath, QString* errorMessage) const
{
    QVector<IoPoint> points = m_repository.load(filePath, errorMessage);
    if (points.isEmpty()) {
        return m_repository.defaultPoints();
    }
    return points;
}

bool IoConfigurationService::savePoints(const QString& filePath, const QVector<IoPoint>& points, QString* errorMessage) const
{
    return m_repository.save(filePath, points, errorMessage);
}

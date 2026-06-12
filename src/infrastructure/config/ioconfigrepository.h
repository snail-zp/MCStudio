#pragma once

#include <QString>
#include <QVector>

#include "domain/models/iopoint.h"

class IoConfigRepository
{
public:
    QVector<IoPoint> load(const QString& filePath, QString* errorMessage) const;
    bool save(const QString& filePath, const QVector<IoPoint>& points, QString* errorMessage) const;
    QVector<IoPoint> defaultPoints() const;
};

#pragma once

#include <QString>

#include "domain/models/commoninterfacecommand.h"

class CommonInterfaceRepository
{
public:
    CommonInterfaceConfig load(const QString& filePath, QString* errorMessage) const;
    bool save(const QString& filePath, const CommonInterfaceConfig& config, QString* errorMessage) const;
};

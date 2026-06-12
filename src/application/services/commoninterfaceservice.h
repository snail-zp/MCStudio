#pragma once

#include <QString>

#include "domain/models/commoninterfacecommand.h"
#include "infrastructure/config/commoninterfacerepository.h"

class CommonInterfaceService
{
public:
    CommonInterfaceConfig loadConfig(const QString& filePath, QString* errorMessage) const;
    bool saveConfig(const QString& filePath, const CommonInterfaceConfig& config, QString* errorMessage) const;

private:
    CommonInterfaceRepository m_repository;
};

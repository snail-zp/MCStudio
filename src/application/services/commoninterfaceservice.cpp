#include "application/services/commoninterfaceservice.h"

CommonInterfaceConfig CommonInterfaceService::loadConfig(const QString& filePath, QString* errorMessage) const
{
    return m_repository.load(filePath, errorMessage);
}

bool CommonInterfaceService::saveConfig(const QString& filePath,
                                        const CommonInterfaceConfig& config,
                                        QString* errorMessage) const
{
    return m_repository.save(filePath, config, errorMessage);
}

#include "application/services/commoninterfaceworkflowservice.h"

const CommonInterfaceMaterialTransferStation* CommonInterfaceWorkflowService::findStationByName(
    const CommonInterfaceMaterialTransferConfig& config,
    const QString& stationName) const
{
    for (const CommonInterfaceMaterialTransferStation& station : config.stations) {
        if (station.stationName.compare(stationName, Qt::CaseInsensitive) == 0) {
            return &station;
        }
    }
    return nullptr;
}

bool CommonInterfaceWorkflowService::isMaterialOnRobot(const CommonInterfaceMaterialTransferConfig& config) const
{
    const QString robotName = config.robotName.trimmed();
    return !robotName.isEmpty()
        && config.currentMaterialLocation.compare(robotName, Qt::CaseInsensitive) == 0;
}

bool CommonInterfaceWorkflowService::isMaterialAtStation(const CommonInterfaceMaterialTransferConfig& config,
                                                         const CommonInterfaceMaterialTransferStation& station) const
{
    return config.currentMaterialLocation.compare(station.stationName, Qt::CaseInsensitive) == 0;
}

bool CommonInterfaceWorkflowService::canTransferToRobot(const CommonInterfaceMaterialTransferConfig& config,
                                                        QString* errorMessage) const
{
    const QString robotName = config.robotName.trimmed();
    if (robotName.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Robot");
        }
        return false;
    }
    return true;
}

const CommonInterfaceMaterialTransferStation* CommonInterfaceWorkflowService::currentSourceStation(
    const CommonInterfaceMaterialTransferConfig& config) const
{
    const QString currentLocation = config.currentMaterialLocation.trimmed();
    return findStationByName(config, currentLocation);
}

void CommonInterfaceWorkflowService::moveMaterialToRobot(CommonInterfaceMaterialTransferConfig* config) const
{
    if (!config) {
        return;
    }
    config->currentMaterialLocation = config->robotName.trimmed();
}

void CommonInterfaceWorkflowService::moveMaterialToStation(CommonInterfaceMaterialTransferConfig* config,
                                                           const QString& stationName) const
{
    if (!config) {
        return;
    }
    config->currentMaterialLocation = stationName;
}

#pragma once

#include "domain/models/commoninterfacecommand.h"

class CommonInterfaceWorkflowService
{
public:
    const CommonInterfaceMaterialTransferStation* findStationByName(
        const CommonInterfaceMaterialTransferConfig& config,
        const QString& stationName) const;

    bool isMaterialOnRobot(const CommonInterfaceMaterialTransferConfig& config) const;
    bool isMaterialAtStation(const CommonInterfaceMaterialTransferConfig& config,
                             const CommonInterfaceMaterialTransferStation& station) const;
    bool canTransferToRobot(const CommonInterfaceMaterialTransferConfig& config,
                            QString* errorMessage) const;
    const CommonInterfaceMaterialTransferStation* currentSourceStation(
        const CommonInterfaceMaterialTransferConfig& config) const;

    void moveMaterialToRobot(CommonInterfaceMaterialTransferConfig* config) const;
    void moveMaterialToStation(CommonInterfaceMaterialTransferConfig* config,
                               const QString& stationName) const;
};

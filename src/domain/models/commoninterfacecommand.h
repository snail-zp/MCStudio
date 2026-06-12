#pragma once

#include <QString>
#include <QVector>

struct CommonInterfaceCommand
{
    QString category;
    QString name;
    QString commandText;
    QString doneVariable;
    int doneBuffer = -1;
    int doneValue = 1;
};

using CommonInterfaceCommandList = QVector<CommonInterfaceCommand>;

struct CommonInterfaceMaterialTransferStation
{
    QString stationName;
    QString pickupCommand;
    QString pickupDoneVariable;
    int pickupDoneValue = 1;
    QString placeCommand;
    QString placeDoneVariable;
    int placeDoneValue = 1;
    QString color = QStringLiteral("#d9e8f5");
};

using CommonInterfaceMaterialTransferStationList = QVector<CommonInterfaceMaterialTransferStation>;

struct CommonInterfaceMaterialTransferConfig
{
    QString robotName;
    QString currentMaterialLocation;
    CommonInterfaceMaterialTransferStationList stations;
};

struct CommonInterfaceConfig
{
    CommonInterfaceCommandList commands;
    CommonInterfaceMaterialTransferConfig materialTransfer;
};

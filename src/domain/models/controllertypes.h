#pragma once

#include <QString>

enum class ControllerBrand
{
    Acs
};

enum class ControllerConnectionType
{
    Simulator,
    EthernetTcp,
    EthernetUdp
};

struct ControllerConnectionOptions
{
    ControllerConnectionType connectionType = ControllerConnectionType::Simulator;
    QString address;
    int port = 0;
};

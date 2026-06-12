#pragma once

#include <QString>

#include "domain/models/commoninterfacecommand.h"

class ControllerService;

class CommonInterfaceExecutionService
{
public:
    explicit CommonInterfaceExecutionService(ControllerService& controllerService);

    bool waitForIntegerVariableValue(const QString& variableName,
                                     int expectedValue,
                                     int timeoutMs,
                                     QString* errorMessage,
                                     int bufferNumber) const;
    bool executeCommand(const CommonInterfaceCommand& command,
                        QString* errorMessage) const;
    bool executeTransferAction(const QString& commandText,
                               const QString& doneVariable,
                               int doneValue,
                               QString* errorMessage) const;

private:
    ControllerService& m_controllerService;
};

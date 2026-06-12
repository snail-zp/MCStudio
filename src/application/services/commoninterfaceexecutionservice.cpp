#include "application/services/commoninterfaceexecutionservice.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QThread>

#include "application/services/controllerservice.h"

CommonInterfaceExecutionService::CommonInterfaceExecutionService(ControllerService& controllerService)
    : m_controllerService(controllerService)
{
}

bool CommonInterfaceExecutionService::waitForIntegerVariableValue(const QString& variableName,
                                                                  int expectedValue,
                                                                  int timeoutMs,
                                                                  QString* errorMessage,
                                                                  int bufferNumber) const
{
    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    while (QDateTime::currentMSecsSinceEpoch() < deadline) {
        int actualValue = 0;
        if (!m_controllerService.readIntegerVariable(variableName, &actualValue, errorMessage, bufferNumber)) {
            return false;
        }
        if (actualValue == expectedValue) {
            return true;
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        QThread::msleep(100);
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Timed out waiting for %1 in buffer %2 = %3")
                            .arg(variableName)
                            .arg(bufferNumber)
                            .arg(expectedValue);
    }
    return false;
}

bool CommonInterfaceExecutionService::executeCommand(const CommonInterfaceCommand& command,
                                                     QString* errorMessage) const
{
    if (!m_controllerService.isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Controller is not connected");
        }
        return false;
    }

    if (!command.doneVariable.isEmpty()) {
        if (!m_controllerService.writeIntegerVariable(command.doneVariable, 0, errorMessage, command.doneBuffer)) {
            return false;
        }
    }

    QString response;
    if (!m_controllerService.executeCommand(command.commandText, &response, errorMessage)) {
        return false;
    }

    if (!command.doneVariable.isEmpty()) {
        if (!waitForIntegerVariableValue(command.doneVariable, command.doneValue, 30000, errorMessage, command.doneBuffer)) {
            return false;
        }
    }

    return true;
}

bool CommonInterfaceExecutionService::executeTransferAction(const QString& commandText,
                                                            const QString& doneVariable,
                                                            int doneValue,
                                                            QString* errorMessage) const
{
    if (commandText.trimmed().isEmpty()) {
        return true;
    }

    if (!m_controllerService.isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Controller is not connected");
        }
        return false;
    }

    if (!doneVariable.trimmed().isEmpty()) {
        if (!m_controllerService.writeIntegerVariable(doneVariable, 0, errorMessage, -1)) {
            return false;
        }
    }

    QString response;
    if (!m_controllerService.executeCommand(commandText, &response, errorMessage)) {
        return false;
    }

    if (!doneVariable.trimmed().isEmpty()) {
        if (!waitForIntegerVariableValue(doneVariable, doneValue, 30000, errorMessage, -1)) {
            return false;
        }
    }

    return true;
}

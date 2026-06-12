#include "application/services/workstationcalibrationexecutionservice.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>

#include "application/services/controllerservice.h"

WorkstationCalibrationExecutionService::WorkstationCalibrationExecutionService(ControllerService& controllerService)
    : m_controllerService(controllerService)
{
}

bool WorkstationCalibrationExecutionService::waitForIntegerVariableValue(const QString& variableName,
                                                                         int bufferNumber,
                                                                         int expectedValue,
                                                                         int timeoutMs,
                                                                         QString* errorMessage) const
{
    if (variableName.trimmed().isEmpty()) {
        return true;
    }

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        int currentValue = 0;
        QString readError;
        if (m_controllerService.readIntegerVariable(variableName, &currentValue, &readError, bufferNumber) && currentValue == expectedValue) {
            return true;
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 20);
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Timed out waiting for %1 to become %2").arg(variableName).arg(expectedValue);
    }
    return false;
}

bool WorkstationCalibrationExecutionService::waitForAxisPosition(int axisNumber,
                                                                 double targetPosition,
                                                                 double tolerance,
                                                                 int timeoutMs,
                                                                 QString* errorMessage) const
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        double currentPosition = 0.0;
        QString readError;
        if (m_controllerService.readAxisPosition(axisNumber, &currentPosition, &readError)) {
            if (qAbs(currentPosition - targetPosition) <= tolerance) {
                return true;
            }
        }
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 20);
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Timed out waiting for axis %1 to reach %2")
                            .arg(axisNumber)
                            .arg(targetPosition, 0, 'f', 3);
    }
    return false;
}

bool WorkstationCalibrationExecutionService::executeCommandWithCompletion(const QString& commandText,
                                                                          const QString& doneVariable,
                                                                          int doneBuffer,
                                                                          int doneValue,
                                                                          QString* errorMessage) const
{
    if (!m_controllerService.isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Controller is not connected");
        }
        return false;
    }

    if (!doneVariable.trimmed().isEmpty()) {
        QString writeError;
        if (!m_controllerService.writeIntegerVariable(doneVariable, 0, &writeError, doneBuffer)) {
            if (errorMessage) {
                *errorMessage = writeError;
            }
            return false;
        }
    }

    QString response;
    if (!m_controllerService.executeCommand(commandText, &response, errorMessage)) {
        return false;
    }

    return waitForIntegerVariableValue(doneVariable, doneBuffer, doneValue, 8000, errorMessage);
}

bool WorkstationCalibrationExecutionService::moveAxisRelativeAndWait(int axisNumber,
                                                                     double stepDistance,
                                                                     double speedValue,
                                                                     double targetPosition,
                                                                     double tolerance,
                                                                     int timeoutMs,
                                                                     QString* errorMessage) const
{
    if (!m_controllerService.moveAxisRelative(axisNumber, stepDistance, speedValue, errorMessage)) {
        return false;
    }

    return waitForAxisPosition(axisNumber, targetPosition, tolerance, timeoutMs, errorMessage);
}

bool WorkstationCalibrationExecutionService::moveAxisToPositionAndWait(int axisNumber,
                                                                       double targetPosition,
                                                                       double speedValue,
                                                                       double tolerance,
                                                                       int timeoutMs,
                                                                       QString* errorMessage) const
{
    if (!m_controllerService.moveAxisToPosition(axisNumber, targetPosition, speedValue, errorMessage)) {
        return false;
    }

    return waitForAxisPosition(axisNumber, targetPosition, tolerance, timeoutMs, errorMessage);
}

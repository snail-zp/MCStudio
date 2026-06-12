#pragma once

#include <QString>

class ControllerService;

class WorkstationCalibrationExecutionService
{
public:
    explicit WorkstationCalibrationExecutionService(ControllerService& controllerService);

    bool waitForIntegerVariableValue(const QString& variableName,
                                     int bufferNumber,
                                     int expectedValue,
                                     int timeoutMs,
                                     QString* errorMessage) const;
    bool waitForAxisPosition(int axisNumber,
                             double targetPosition,
                             double tolerance,
                             int timeoutMs,
                             QString* errorMessage) const;
    bool executeCommandWithCompletion(const QString& commandText,
                                      const QString& doneVariable,
                                      int doneBuffer,
                                      int doneValue,
                                      QString* errorMessage) const;
    bool moveAxisRelativeAndWait(int axisNumber,
                                 double stepDistance,
                                 double speedValue,
                                 double targetPosition,
                                 double tolerance,
                                 int timeoutMs,
                                 QString* errorMessage) const;
    bool moveAxisToPositionAndWait(int axisNumber,
                                   double targetPosition,
                                   double speedValue,
                                   double tolerance,
                                   int timeoutMs,
                                   QString* errorMessage) const;

private:
    ControllerService& m_controllerService;
};

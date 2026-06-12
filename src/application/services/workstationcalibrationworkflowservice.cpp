#include "application/services/workstationcalibrationworkflowservice.h"

#include <QSet>

#include "application/services/controllerservice.h"
#include "application/services/workstationcalibrationservice.h"

WorkstationCalibrationWorkflowService::WorkstationCalibrationWorkflowService(ControllerService& controllerService,
                                                                             WorkstationCalibrationService& calibrationService)
    : m_controllerService(controllerService)
    , m_calibrationService(calibrationService)
{
}

WorkstationCalibration* WorkstationCalibrationWorkflowService::findCalibration(QVector<WorkstationCalibration>* calibrations,
                                                                               const QString& workstationName) const
{
    if (!calibrations) {
        return nullptr;
    }

    for (WorkstationCalibration& calibration : *calibrations) {
        if (calibration.calibrationName == workstationName) {
            return &calibration;
        }
    }
    return nullptr;
}

const WorkstationCalibration* WorkstationCalibrationWorkflowService::findCalibration(const QVector<WorkstationCalibration>& calibrations,
                                                                                     const QString& workstationName) const
{
    for (const WorkstationCalibration& calibration : calibrations) {
        if (calibration.calibrationName == workstationName) {
            return &calibration;
        }
    }
    return nullptr;
}

CalibrationAxis* WorkstationCalibrationWorkflowService::findAxis(QVector<WorkstationCalibration>* calibrations,
                                                                 const QString& workstationName,
                                                                 const QString& moduleKey,
                                                                 const QString& axisName) const
{
    if (!calibrations) {
        return nullptr;
    }

    for (WorkstationCalibration& calibration : *calibrations) {
        if (calibration.calibrationName != workstationName) {
            continue;
        }

        for (CalibrationModule& module : calibration.modules) {
            if (module.key != moduleKey) {
                continue;
            }

            for (CalibrationAxis& axis : module.axes) {
                if (axis.name.compare(axisName, Qt::CaseInsensitive) == 0) {
                    return &axis;
                }
            }
        }
    }

    return nullptr;
}

const CalibrationAxis* WorkstationCalibrationWorkflowService::findAxis(const QVector<WorkstationCalibration>& calibrations,
                                                                       const QString& workstationName,
                                                                       const QString& moduleKey,
                                                                       const QString& axisName) const
{
    for (const WorkstationCalibration& calibration : calibrations) {
        if (calibration.calibrationName != workstationName) {
            continue;
        }

        for (const CalibrationModule& module : calibration.modules) {
            if (module.key != moduleKey) {
                continue;
            }

            for (const CalibrationAxis& axis : module.axes) {
                if (axis.name.compare(axisName, Qt::CaseInsensitive) == 0) {
                    return &axis;
                }
            }
        }
    }

    return nullptr;
}

WorkstationCalibrationWorkflowService::AxisMatch WorkstationCalibrationWorkflowService::findAxisByName(const WorkstationCalibration& calibration,
                                                                                                        const QString& axisName) const
{
    for (const CalibrationModule& module : calibration.modules) {
        for (const CalibrationAxis& axis : module.axes) {
            if (axis.name.compare(axisName, Qt::CaseInsensitive) == 0) {
                return {module.key, &axis};
            }
        }
    }
    return {};
}

bool WorkstationCalibrationWorkflowService::readAxisEditorState(const QVector<WorkstationCalibration>& calibrations,
                                                                const QString& workstationName,
                                                                const QString& moduleKey,
                                                                const QString& axisName,
                                                                AxisEditorState* state) const
{
    if (!state) {
        return false;
    }

    const CalibrationAxis* axis = findAxis(calibrations, workstationName, moduleKey, axisName);
    if (!axis) {
        return false;
    }

    state->axisNumber = axis->axisNumber;
    state->defaultSpeed = axis->defaultSpeed;
    state->defaultStep = axis->defaultStep;
    state->unit = axis->unit;
    state->moduleName.clear();

    const WorkstationCalibration* calibration = findCalibration(calibrations, workstationName);
    if (!calibration) {
        return true;
    }

    for (const CalibrationModule& module : calibration->modules) {
        if (module.key == moduleKey) {
            state->moduleName = module.name;
            break;
        }
    }

    return true;
}

bool WorkstationCalibrationWorkflowService::updateAxisDefaultSpeed(QVector<WorkstationCalibration>* calibrations,
                                                                   const QString& workstationName,
                                                                   const QString& moduleKey,
                                                                   const QString& axisName,
                                                                   double value) const
{
    CalibrationAxis* axis = findAxis(calibrations, workstationName, moduleKey, axisName);
    if (!axis) {
        return false;
    }

    axis->defaultSpeed = value;
    return true;
}

bool WorkstationCalibrationWorkflowService::updateAxisDefaultStep(QVector<WorkstationCalibration>* calibrations,
                                                                  const QString& workstationName,
                                                                  const QString& moduleKey,
                                                                  const QString& axisName,
                                                                  double value) const
{
    CalibrationAxis* axis = findAxis(calibrations, workstationName, moduleKey, axisName);
    if (!axis) {
        return false;
    }

    axis->defaultStep = value;
    return true;
}

double WorkstationCalibrationWorkflowService::jogSpeedFor(const WorkstationCalibration& calibration,
                                                          const CalibrationAxis& axis,
                                                          bool useHighSpeed) const
{
    Q_UNUSED(axis);
    return useHighSpeed ? calibration.jogSpeedHigh : calibration.jogSpeedLow;
}

bool WorkstationCalibrationWorkflowService::prepareJogMove(const QVector<WorkstationCalibration>& calibrations,
                                                           const QString& workstationName,
                                                           const QString& moduleKey,
                                                           const QString& axisName,
                                                           double direction,
                                                           bool useHighSpeed,
                                                           JogMoveRequest* request,
                                                           QString* errorMessage) const
{
    if (!request) {
        return false;
    }

    const WorkstationCalibration* calibration = findCalibration(calibrations, workstationName);
    const CalibrationAxis* matchedAxis = nullptr;
    for (const WorkstationCalibration& item : calibrations) {
        if (item.calibrationName != workstationName) {
            continue;
        }
        for (const CalibrationModule& module : item.modules) {
            if (module.key != moduleKey) {
                continue;
            }
            for (const CalibrationAxis& axis : module.axes) {
                if (axis.name.compare(axisName, Qt::CaseInsensitive) == 0) {
                    matchedAxis = &axis;
                    break;
                }
            }
            if (matchedAxis) {
                break;
            }
        }
        if (matchedAxis) {
            break;
        }
    }

    if (!matchedAxis) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Axis %1 was not found in calibration config").arg(axisName);
        }
        return false;
    }

    const double stepValue = calibration ? calibration->unifiedStep : matchedAxis->defaultStep;
    const double speedValue = calibration ? jogSpeedFor(*calibration, *matchedAxis, useHighSpeed) : matchedAxis->defaultSpeed;
    const double stepDistance = stepValue * direction;
    double currentPosition = 0.0;
    if (!m_controllerService.readAxisPosition(matchedAxis->axisNumber, &currentPosition, errorMessage)) {
        return false;
    }

    request->axisName = matchedAxis->name;
    request->axisUnit = matchedAxis->unit;
    request->axisNumber = matchedAxis->axisNumber;
    request->stepDistance = stepDistance;
    request->speedValue = speedValue;
    request->targetPosition = currentPosition + stepDistance;
    request->tolerance = qMax(0.01, qAbs(stepValue) * 0.2);
    return true;
}

bool WorkstationCalibrationWorkflowService::prepareStartPositionRequest(const QVector<WorkstationCalibration>& calibrations,
                                                                        const QString& workstationName,
                                                                        StartPositionRequest* request,
                                                                        QString* errorMessage) const
{
    if (!request) {
        return false;
    }

    const WorkstationCalibration* calibration = findCalibration(calibrations, workstationName);
    if (!calibration) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Workstation %1 was not found").arg(workstationName);
        }
        return false;
    }

    if (calibration->startPositionCommand.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Start-position command is empty");
        }
        return false;
    }

    request->command = calibration->startPositionCommand;
    request->doneVariable = calibration->startPositionDoneVariable;
    request->doneBuffer = calibration->startPositionDoneBuffer;
    request->doneValue = calibration->startPositionDoneValue;
    return true;
}

bool WorkstationCalibrationWorkflowService::captureAxisPositions(const WorkstationCalibration& calibration,
                                                                 QMap<QString, double>* positions,
                                                                 QString* errorMessage) const
{
    if (!positions) {
        return false;
    }

    positions->clear();
    QString lastError;
    for (const CalibrationModule& module : calibration.modules) {
        for (const CalibrationAxis& axis : module.axes) {
            double position = 0.0;
            QString readError;
            if (m_controllerService.readAxisPosition(axis.axisNumber, &position, &readError)) {
                positions->insert(axis.name.toUpper(), position);
            } else {
                lastError = readError;
            }
        }
    }

    if (!lastError.isEmpty() && errorMessage) {
        *errorMessage = lastError;
    }
    return true;
}

bool WorkstationCalibrationWorkflowService::readCurrentMoveSequencePosition(const WorkstationCalibration& calibration,
                                                                            int selectedIndex,
                                                                            PendingStepUpdate* update,
                                                                            QString* errorMessage) const
{
    if (!update || selectedIndex < 0 || selectedIndex >= calibration.moveSequence.size()) {
        return false;
    }

    const CalibrationMoveStep& step = calibration.moveSequence.at(selectedIndex);
    const AxisMatch axisMatch = findAxisByName(calibration, step.axisName);
    if (!axisMatch.axis) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Axis %1 was not found in move sequence").arg(step.axisName);
        }
        return false;
    }

    double currentPosition = 0.0;
    if (!m_controllerService.readAxisPosition(axisMatch.axis->axisNumber, &currentPosition, errorMessage)) {
        return false;
    }

    update->stepIndex = selectedIndex;
    update->axisName = step.axisName;
    update->unit = axisMatch.axis->unit;
    update->position = currentPosition;
    return true;
}

bool WorkstationCalibrationWorkflowService::collectPendingMoveSequenceUpdates(const WorkstationCalibration& calibration,
                                                                              int selectedIndex,
                                                                              QVector<PendingStepUpdate>* updates,
                                                                              QString* errorMessage) const
{
    if (!updates) {
        return false;
    }

    updates->clear();
    QSet<QString> visitedAxes;

    for (int stepIndex = selectedIndex; stepIndex >= 0; --stepIndex) {
        const CalibrationMoveStep& step = calibration.moveSequence.at(stepIndex);
        const QString axisKeyName = step.axisName.trimmed().toUpper();
        if (axisKeyName.isEmpty() || visitedAxes.contains(axisKeyName)) {
            continue;
        }

        PendingStepUpdate update;
        if (!readCurrentMoveSequencePosition(calibration, stepIndex, &update, errorMessage)) {
            return false;
        }

        updates->push_back(update);
        visitedAxes.insert(axisKeyName);
    }

    return true;
}

bool WorkstationCalibrationWorkflowService::saveCalibrations(const QString& filePath,
                                                             const QVector<WorkstationCalibration>& calibrations,
                                                             QString* errorMessage) const
{
    return m_calibrationService.saveCalibrations(filePath, calibrations, errorMessage);
}

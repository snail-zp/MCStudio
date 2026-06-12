#pragma once

#include <QMap>
#include <QVector>

#include "domain/models/workstationcalibration.h"

class ControllerService;
class WorkstationCalibrationService;

class WorkstationCalibrationWorkflowService
{
public:
    struct AxisMatch
    {
        QString moduleKey;
        const CalibrationAxis* axis = nullptr;
    };

    struct PendingStepUpdate
    {
        int stepIndex = -1;
        QString axisName;
        QString unit;
        double position = 0.0;
    };

    struct AxisEditorState
    {
        int axisNumber = 0;
        double defaultSpeed = 0.0;
        double defaultStep = 0.0;
        QString unit;
        QString moduleName;
    };

    struct JogMoveRequest
    {
        QString axisName;
        QString axisUnit;
        int axisNumber = 0;
        double stepDistance = 0.0;
        double speedValue = 0.0;
        double targetPosition = 0.0;
        double tolerance = 0.0;
    };

    struct StartPositionRequest
    {
        QString command;
        QString doneVariable;
        int doneBuffer = -1;
        int doneValue = 0;
    };

    WorkstationCalibrationWorkflowService(ControllerService& controllerService,
                                         WorkstationCalibrationService& calibrationService);

    WorkstationCalibration* findCalibration(QVector<WorkstationCalibration>* calibrations, const QString& workstationName) const;
    const WorkstationCalibration* findCalibration(const QVector<WorkstationCalibration>& calibrations, const QString& workstationName) const;
    CalibrationAxis* findAxis(QVector<WorkstationCalibration>* calibrations,
                              const QString& workstationName,
                              const QString& moduleKey,
                              const QString& axisName) const;
    const CalibrationAxis* findAxis(const QVector<WorkstationCalibration>& calibrations,
                                    const QString& workstationName,
                                    const QString& moduleKey,
                                    const QString& axisName) const;
    AxisMatch findAxisByName(const WorkstationCalibration& calibration, const QString& axisName) const;
    bool readAxisEditorState(const QVector<WorkstationCalibration>& calibrations,
                             const QString& workstationName,
                             const QString& moduleKey,
                             const QString& axisName,
                             AxisEditorState* state) const;
    bool updateAxisDefaultSpeed(QVector<WorkstationCalibration>* calibrations,
                                const QString& workstationName,
                                const QString& moduleKey,
                                const QString& axisName,
                                double value) const;
    bool updateAxisDefaultStep(QVector<WorkstationCalibration>* calibrations,
                               const QString& workstationName,
                               const QString& moduleKey,
                               const QString& axisName,
                               double value) const;
    double jogSpeedFor(const WorkstationCalibration& calibration, const CalibrationAxis& axis, bool useHighSpeed) const;
    bool prepareJogMove(const QVector<WorkstationCalibration>& calibrations,
                        const QString& workstationName,
                        const QString& moduleKey,
                        const QString& axisName,
                        double direction,
                        bool useHighSpeed,
                        JogMoveRequest* request,
                        QString* errorMessage) const;
    bool prepareStartPositionRequest(const QVector<WorkstationCalibration>& calibrations,
                                     const QString& workstationName,
                                     StartPositionRequest* request,
                                     QString* errorMessage) const;
    bool captureAxisPositions(const WorkstationCalibration& calibration,
                              QMap<QString, double>* positions,
                              QString* errorMessage) const;
    bool readCurrentMoveSequencePosition(const WorkstationCalibration& calibration,
                                         int selectedIndex,
                                         PendingStepUpdate* update,
                                         QString* errorMessage) const;
    bool collectPendingMoveSequenceUpdates(const WorkstationCalibration& calibration,
                                           int selectedIndex,
                                           QVector<PendingStepUpdate>* updates,
                                           QString* errorMessage) const;
    bool saveCalibrations(const QString& filePath,
                          const QVector<WorkstationCalibration>& calibrations,
                          QString* errorMessage) const;

private:
    ControllerService& m_controllerService;
    WorkstationCalibrationService& m_calibrationService;
};

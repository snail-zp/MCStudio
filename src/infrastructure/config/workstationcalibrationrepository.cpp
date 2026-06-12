#include "infrastructure/config/workstationcalibrationrepository.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace
{
CalibrationAxis parseAxis(const QJsonObject& object)
{
    CalibrationAxis axis;
    axis.name = object.value(QStringLiteral("name")).toString();
    axis.axisNumber = object.value(QStringLiteral("axisNumber")).toInt();
    axis.defaultSpeed = object.value(QStringLiteral("defaultSpeed")).toDouble(10.0);
    axis.defaultStep = object.value(QStringLiteral("defaultStep")).toDouble(1.0);
    axis.unit = object.value(QStringLiteral("unit")).toString(QStringLiteral("mm"));
    return axis;
}

CalibrationModule parseModule(const QString& key, const QJsonObject& object)
{
    CalibrationModule module;
    module.key = key;
    module.name = object.value(QStringLiteral("name")).toString(key);
    module.description = object.value(QStringLiteral("description")).toString();

    const QJsonArray axesArray = object.value(QStringLiteral("axes")).toArray();
    module.axes.reserve(axesArray.size());
    for (const QJsonValue& value : axesArray) {
        const CalibrationAxis axis = parseAxis(value.toObject());
        if (!axis.name.isEmpty() && axis.axisNumber > 0) {
            module.axes.push_back(axis);
        }
    }

    const QJsonObject operationsObject = object.value(QStringLiteral("operations")).toObject();
    for (auto it = operationsObject.begin(); it != operationsObject.end(); ++it) {
        CalibrationCommandAction action;
        if (it.value().isObject()) {
            const QJsonObject actionObject = it.value().toObject();
            action.command = actionObject.value(QStringLiteral("command")).toString();
            action.doneVariable = actionObject.value(QStringLiteral("doneVariable")).toString();
            action.doneBuffer = actionObject.value(QStringLiteral("doneBuffer")).toInt(-1);
            action.doneValue = actionObject.value(QStringLiteral("doneValue")).toInt(1);
        } else {
            action.command = it.value().toString();
        }
        module.operations.insert(it.key(), action);
    }

    return module;
}

QVector<CalibrationMoveStep> parseMoveSequence(const QJsonObject& object)
{
    QVector<CalibrationMoveStep> result;

    const QJsonArray sequenceArray = object.value(QStringLiteral("moveSequence")).toArray();
    const QJsonArray positionArray = object.value(QStringLiteral("targetPositions")).toArray();
    const QJsonArray sensorArray = object.value(QStringLiteral("arrivalSensors")).toArray();
    const int count = qMin(sequenceArray.size(), positionArray.size());
    result.reserve(count);

    for (int index = 0; index < count; ++index) {
        CalibrationMoveStep step;
        step.axisName = sequenceArray.at(index).toString().trimmed();
        step.targetPosition = positionArray.at(index).toDouble();
        if (index < sensorArray.size()) {
            step.arrivalSensorVariable = sensorArray.at(index).toString().trimmed();
        }
        if (!step.axisName.isEmpty()) {
            result.push_back(step);
        }
    }

    return result;
}
}

QVector<WorkstationCalibration> WorkstationCalibrationRepository::load(const QString& filePath, QString* errorMessage) const
{
    QFile file(filePath);
    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Calibration config file does not exist: %1").arg(filePath);
        }
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open calibration config: %1").arg(file.errorString());
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Calibration JSON parse failed: %1").arg(parseError.errorString());
        }
        return {};
    }

    const QJsonArray workstationArray = document.object().value(QStringLiteral("workstationCalibrations")).toArray();
    QVector<WorkstationCalibration> calibrations;
    calibrations.reserve(workstationArray.size());

    for (const QJsonValue& workstationValue : workstationArray) {
        const QJsonObject workstationObject = workstationValue.toObject();
        WorkstationCalibration calibration;
        calibration.calibrationName = workstationObject.value(QStringLiteral("calibrationName")).toString();
        calibration.description = workstationObject.value(QStringLiteral("description")).toString();
        calibration.moveSequence = parseMoveSequence(workstationObject);
        calibration.startPositionCommand = workstationObject.value(QStringLiteral("startPositionCommand")).toString();
        calibration.startPositionDoneVariable = workstationObject.value(QStringLiteral("startPositionDoneVariable")).toString();
        calibration.startPositionDoneBuffer = workstationObject.value(QStringLiteral("startPositionDoneBuffer")).toInt(-1);
        calibration.startPositionDoneValue = workstationObject.value(QStringLiteral("startPositionDoneValue")).toInt(1);
        calibration.unifiedStep = workstationObject.value(QStringLiteral("unifiedStep")).toDouble(1.0);
        calibration.jogSpeedLow = workstationObject.value(QStringLiteral("jogSpeedLow")).toDouble(10.0);
        calibration.jogSpeedHigh = workstationObject.value(QStringLiteral("jogSpeedHigh")).toDouble(50.0);

        for (auto it = workstationObject.begin(); it != workstationObject.end(); ++it) {
            if (it.key() == QStringLiteral("calibrationName")
                || it.key() == QStringLiteral("description")
                || it.key() == QStringLiteral("moveSequence")
                || it.key() == QStringLiteral("targetPositions")
                || it.key() == QStringLiteral("arrivalSensors")
                || it.key() == QStringLiteral("startPositionCommand")
                || it.key() == QStringLiteral("unifiedStep")
                || it.key() == QStringLiteral("jogSpeedLow")
                || it.key() == QStringLiteral("jogSpeedHigh")) {
                continue;
            }

            if (!it.value().isObject()) {
                continue;
            }

            CalibrationModule module = parseModule(it.key(), it.value().toObject());
            if (!module.name.isEmpty()) {
                calibration.modules.push_back(module);
            }
        }

        if (!calibration.calibrationName.isEmpty()) {
            calibrations.push_back(calibration);
        }
    }

    if (calibrations.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No valid workstation calibration definitions were found");
    }

    return calibrations;
}

bool WorkstationCalibrationRepository::save(const QString& filePath,
                                            const QVector<WorkstationCalibration>& calibrations,
                                            QString* errorMessage) const
{
    QJsonArray workstationArray;

    for (const WorkstationCalibration& calibration : calibrations) {
        QJsonObject workstationObject;
        workstationObject.insert(QStringLiteral("calibrationName"), calibration.calibrationName);
        workstationObject.insert(QStringLiteral("description"), calibration.description);
        if (!calibration.startPositionCommand.isEmpty()) {
            workstationObject.insert(QStringLiteral("startPositionCommand"), calibration.startPositionCommand);
        }
        if (!calibration.startPositionDoneVariable.isEmpty()) {
            workstationObject.insert(QStringLiteral("startPositionDoneVariable"), calibration.startPositionDoneVariable);
        }
        workstationObject.insert(QStringLiteral("startPositionDoneBuffer"), calibration.startPositionDoneBuffer);
        workstationObject.insert(QStringLiteral("startPositionDoneValue"), calibration.startPositionDoneValue);
        workstationObject.insert(QStringLiteral("unifiedStep"), calibration.unifiedStep);
        workstationObject.insert(QStringLiteral("jogSpeedLow"), calibration.jogSpeedLow);
        workstationObject.insert(QStringLiteral("jogSpeedHigh"), calibration.jogSpeedHigh);
        if (!calibration.moveSequence.isEmpty()) {
            QJsonArray sequenceArray;
            QJsonArray positionArray;
            QJsonArray sensorArray;
            for (const CalibrationMoveStep& step : calibration.moveSequence) {
                sequenceArray.append(step.axisName);
                positionArray.append(step.targetPosition);
                sensorArray.append(step.arrivalSensorVariable);
            }
            workstationObject.insert(QStringLiteral("moveSequence"), sequenceArray);
            workstationObject.insert(QStringLiteral("targetPositions"), positionArray);
            workstationObject.insert(QStringLiteral("arrivalSensors"), sensorArray);
        }

        for (const CalibrationModule& module : calibration.modules) {
            QJsonObject moduleObject;
            moduleObject.insert(QStringLiteral("name"), module.name);
            moduleObject.insert(QStringLiteral("description"), module.description);

            QJsonArray axesArray;
            for (const CalibrationAxis& axis : module.axes) {
                QJsonObject axisObject;
                axisObject.insert(QStringLiteral("name"), axis.name);
                axisObject.insert(QStringLiteral("axisNumber"), axis.axisNumber);
                axisObject.insert(QStringLiteral("defaultSpeed"), axis.defaultSpeed);
                axisObject.insert(QStringLiteral("defaultStep"), axis.defaultStep);
                axisObject.insert(QStringLiteral("unit"), axis.unit);
                axesArray.append(axisObject);
            }
            moduleObject.insert(QStringLiteral("axes"), axesArray);

            QJsonObject operationsObject;
            for (auto it = module.operations.cbegin(); it != module.operations.cend(); ++it) {
                QJsonObject actionObject;
                actionObject.insert(QStringLiteral("command"), it.value().command);
                if (!it.value().doneVariable.isEmpty()) {
                    actionObject.insert(QStringLiteral("doneVariable"), it.value().doneVariable);
                }
                actionObject.insert(QStringLiteral("doneBuffer"), it.value().doneBuffer);
                actionObject.insert(QStringLiteral("doneValue"), it.value().doneValue);
                operationsObject.insert(it.key(), actionObject);
            }
            moduleObject.insert(QStringLiteral("operations"), operationsObject);

            workstationObject.insert(module.key, moduleObject);
        }

        workstationArray.append(workstationObject);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("workstationCalibrations"), workstationArray);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open calibration config for save: %1").arg(file.errorString());
        }
        return false;
    }

    const QByteArray json = QJsonDocument(rootObject).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write calibration config: %1").arg(file.errorString());
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to commit calibration config: %1").arg(file.errorString());
        }
        return false;
    }

    return true;
}

#include "infrastructure/config/commoninterfacerepository.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace
{
CommonInterfaceCommandList parseCommands(const QJsonArray& commandArray)
{
    CommonInterfaceCommandList commands;
    commands.reserve(commandArray.size());

    for (const QJsonValue& value : commandArray) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        CommonInterfaceCommand command;
        command.category = object.value(QStringLiteral("category")).toString().trimmed();
        command.name = object.value(QStringLiteral("name")).toString().trimmed();
        command.commandText = object.value(QStringLiteral("command")).toString().trimmed();
        command.doneVariable = object.value(QStringLiteral("doneVariable")).toString().trimmed();
        command.doneBuffer = object.value(QStringLiteral("doneBuffer")).toInt(-1);
        command.doneValue = object.value(QStringLiteral("doneValue")).toInt(1);
        if (!command.category.isEmpty() && !command.name.isEmpty() && !command.commandText.isEmpty()) {
            commands.push_back(command);
        }
    }

    return commands;
}

CommonInterfaceMaterialTransferConfig parseMaterialTransfer(const QJsonObject& rootObject)
{
    CommonInterfaceMaterialTransferConfig transferConfig;
    const QJsonObject transferObject = rootObject.value(QStringLiteral("materialTransfer")).toObject();
    transferConfig.robotName = transferObject.value(QStringLiteral("robotName")).toString().trimmed();
    transferConfig.currentMaterialLocation = transferObject.value(QStringLiteral("currentMaterialLocation")).toString().trimmed();
    if (transferConfig.currentMaterialLocation.isEmpty()) {
        transferConfig.currentMaterialLocation = transferObject.value(QStringLiteral("currentBoardStation")).toString().trimmed();
    }

    const QJsonArray stationArray = transferObject.value(QStringLiteral("stations")).toArray();
    transferConfig.stations.reserve(stationArray.size());
    for (const QJsonValue& value : stationArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        CommonInterfaceMaterialTransferStation station;
        station.stationName = object.value(QStringLiteral("stationName")).toString().trimmed();
        station.pickupCommand = object.value(QStringLiteral("pickupCommand")).toString().trimmed();
        station.pickupDoneVariable = object.value(QStringLiteral("pickupDoneVariable")).toString().trimmed();
        station.pickupDoneValue = object.value(QStringLiteral("pickupDoneValue")).toInt(1);
        station.placeCommand = object.value(QStringLiteral("placeCommand")).toString().trimmed();
        station.placeDoneVariable = object.value(QStringLiteral("placeDoneVariable")).toString().trimmed();
        station.placeDoneValue = object.value(QStringLiteral("placeDoneValue")).toInt(1);
        station.color = object.value(QStringLiteral("color")).toString(QStringLiteral("#d9e8f5")).trimmed();
        if (station.pickupDoneVariable.isEmpty()) {
            station.pickupDoneVariable = object.value(QStringLiteral("doneVariable")).toString().trimmed();
        }
        if (station.placeDoneVariable.isEmpty()) {
            station.placeDoneVariable = object.value(QStringLiteral("doneVariable")).toString().trimmed();
        }
        if (!object.contains(QStringLiteral("pickupDoneValue"))) {
            const int legacyDoneValue = object.value(QStringLiteral("doneValue")).toInt(1);
            station.pickupDoneValue = legacyDoneValue;
        }
        if (!object.contains(QStringLiteral("placeDoneValue"))) {
            const int legacyDoneValue = object.value(QStringLiteral("doneValue")).toInt(1);
            station.placeDoneValue = legacyDoneValue;
        }
        if (!station.stationName.isEmpty() && (!station.pickupCommand.isEmpty() || !station.placeCommand.isEmpty())) {
            transferConfig.stations.push_back(station);
        }
    }

    return transferConfig;
}
}

CommonInterfaceConfig CommonInterfaceRepository::load(const QString& filePath, QString* errorMessage) const
{
    QFile file(filePath);
    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Common interface config file does not exist: %1").arg(filePath);
        }
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open common interface config: %1").arg(file.errorString());
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Common interface JSON parse failed: %1").arg(parseError.errorString());
        }
        return {};
    }

    const QJsonObject rootObject = document.object();
    CommonInterfaceConfig config;
    config.commands = parseCommands(rootObject.value(QStringLiteral("commands")).toArray());
    config.materialTransfer = parseMaterialTransfer(rootObject);

    if (config.commands.isEmpty() && config.materialTransfer.stations.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No valid common interface commands were found");
    }

    return config;
}

bool CommonInterfaceRepository::save(const QString& filePath,
                                     const CommonInterfaceConfig& config,
                                     QString* errorMessage) const
{
    QJsonArray commandArray;
    for (const CommonInterfaceCommand& command : config.commands) {
        QJsonObject object;
        object.insert(QStringLiteral("category"), command.category);
        object.insert(QStringLiteral("name"), command.name);
        object.insert(QStringLiteral("command"), command.commandText);
        if (!command.doneVariable.isEmpty()) {
            object.insert(QStringLiteral("doneVariable"), command.doneVariable);
            object.insert(QStringLiteral("doneBuffer"), command.doneBuffer);
        }
        object.insert(QStringLiteral("doneValue"), command.doneValue);
        commandArray.push_back(object);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("commands"), commandArray);

    QJsonObject transferObject;
    if (!config.materialTransfer.robotName.isEmpty()) {
        transferObject.insert(QStringLiteral("robotName"), config.materialTransfer.robotName);
    }
    if (!config.materialTransfer.currentMaterialLocation.isEmpty()) {
        transferObject.insert(QStringLiteral("currentMaterialLocation"), config.materialTransfer.currentMaterialLocation);
    }

    QJsonArray stationArray;
    for (const CommonInterfaceMaterialTransferStation& station : config.materialTransfer.stations) {
        QJsonObject object;
        object.insert(QStringLiteral("stationName"), station.stationName);
        if (!station.pickupCommand.isEmpty()) {
            object.insert(QStringLiteral("pickupCommand"), station.pickupCommand);
        }
        if (!station.pickupDoneVariable.isEmpty()) {
            object.insert(QStringLiteral("pickupDoneVariable"), station.pickupDoneVariable);
        }
        object.insert(QStringLiteral("pickupDoneValue"), station.pickupDoneValue);
        if (!station.placeCommand.isEmpty()) {
            object.insert(QStringLiteral("placeCommand"), station.placeCommand);
        }
        if (!station.placeDoneVariable.isEmpty()) {
            object.insert(QStringLiteral("placeDoneVariable"), station.placeDoneVariable);
        }
        object.insert(QStringLiteral("placeDoneValue"), station.placeDoneValue);
        if (!station.color.isEmpty()) {
            object.insert(QStringLiteral("color"), station.color);
        }
        stationArray.push_back(object);
    }
    transferObject.insert(QStringLiteral("stations"), stationArray);
    rootObject.insert(QStringLiteral("materialTransfer"), transferObject);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open common interface config for save: %1").arg(file.errorString());
        }
        return false;
    }

    const QByteArray json = QJsonDocument(rootObject).toJson(QJsonDocument::Indented);
    if (file.write(json) != json.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write common interface config: %1").arg(file.errorString());
        }
        return false;
    }

    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to commit common interface config: %1").arg(file.errorString());
        }
        return false;
    }

    return true;
}

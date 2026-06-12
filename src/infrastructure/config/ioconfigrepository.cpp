#include "infrastructure/config/ioconfigrepository.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

namespace
{
QString inferDirection(const QString& ioType)
{
    const QString normalized = ioType.trimmed().toUpper();
    if (normalized == QStringLiteral("DI") || normalized == QStringLiteral("AI")) {
        return QStringLiteral("input");
    }
    if (normalized == QStringLiteral("DO") || normalized == QStringLiteral("AO")) {
        return QStringLiteral("output");
    }
    return QString();
}

IoPoint parseLegacyPoint(const QJsonObject& object)
{
    IoPoint point;
    point.variableName = object.value(QStringLiteral("variable_name")).toString();
    point.id = point.variableName;
    point.name = object.value(QStringLiteral("name")).toString();
    point.module = object.value(QStringLiteral("module")).toString();
    point.ioType = object.value(QStringLiteral("io_type")).toString().trimmed().toUpper();
    point.direction = inferDirection(point.ioType);
    point.unit = object.value(QStringLiteral("unit")).toString();
    point.range = object.value(QStringLiteral("range")).toString();
    point.description = object.value(QStringLiteral("description")).toString();
    return point;
}

IoPoint parseModernPoint(const QJsonObject& object)
{
    IoPoint point;
    point.id = object.value(QStringLiteral("id")).toString();
    point.variableName = object.value(QStringLiteral("variable_name")).toString();
    point.name = object.value(QStringLiteral("name")).toString();
    point.module = object.value(QStringLiteral("module")).toString();
    point.ioType = object.value(QStringLiteral("io_type")).toString().trimmed().toUpper();
    point.direction = object.value(QStringLiteral("direction")).toString().trimmed().toLower();
    point.unit = object.value(QStringLiteral("unit")).toString();
    point.range = object.value(QStringLiteral("range")).toString();
    point.description = object.value(QStringLiteral("description")).toString();

    if (point.direction.isEmpty()) {
        point.direction = inferDirection(point.ioType);
    }
    if (point.variableName.isEmpty()) {
        point.variableName = point.id;
    }
    if (point.id.isEmpty()) {
        point.id = point.variableName;
    }
    return point;
}

bool isValidPoint(const IoPoint& point)
{
    return !point.variableName.isEmpty()
        && !point.module.isEmpty()
        && !point.ioType.isEmpty();
}

QJsonObject toLegacyPointObject(const IoPoint& point)
{
    QJsonObject object;
    object.insert(QStringLiteral("name"), point.name);
    object.insert(QStringLiteral("variable_name"), point.variableName);
    object.insert(QStringLiteral("module"), point.module);
    object.insert(QStringLiteral("io_type"), point.ioType);
    if (!point.unit.isEmpty()) {
        object.insert(QStringLiteral("unit"), point.unit);
    }
    if (!point.range.isEmpty()) {
        object.insert(QStringLiteral("range"), point.range);
    }
    if (!point.description.isEmpty()) {
        object.insert(QStringLiteral("description"), point.description);
    }
    return object;
}

QJsonObject toModernPointObject(const IoPoint& point)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), point.id.isEmpty() ? QStringLiteral("io_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)) : point.id);
    object.insert(QStringLiteral("variable_name"), point.variableName);
    object.insert(QStringLiteral("name"), point.name);
    object.insert(QStringLiteral("module"), point.module);
    object.insert(QStringLiteral("io_type"), point.ioType);
    if (!point.direction.isEmpty()) {
        object.insert(QStringLiteral("direction"), point.direction);
    }
    if (!point.unit.isEmpty()) {
        object.insert(QStringLiteral("unit"), point.unit);
    }
    if (!point.range.isEmpty()) {
        object.insert(QStringLiteral("range"), point.range);
    }
    if (!point.description.isEmpty()) {
        object.insert(QStringLiteral("description"), point.description);
    }
    return object;
}
}

QVector<IoPoint> IoConfigRepository::load(const QString& filePath, QString* errorMessage) const
{
    QFile file(filePath);
    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Config file does not exist: %1").arg(filePath);
        }
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open config file: %1").arg(file.errorString());
        }
        return {};
    }

    QJsonParseError actualError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &actualError);
    if (actualError.error != QJsonParseError::NoError) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("JSON parse failed: %1").arg(actualError.errorString());
        }
        return {};
    }

    const QJsonObject root = document.object();
    QJsonArray pointsArray;
    const bool isLegacy = root.contains(QStringLiteral("io_configuration"));

    if (isLegacy) {
        pointsArray = root.value(QStringLiteral("io_configuration")).toObject().value(QStringLiteral("ios")).toArray();
    } else {
        pointsArray = root.value(QStringLiteral("io_points")).toArray();
    }

    QVector<IoPoint> points;
    points.reserve(pointsArray.size());

    for (const QJsonValue& value : pointsArray) {
        const IoPoint point = isLegacy ? parseLegacyPoint(value.toObject()) : parseModernPoint(value.toObject());
        if (isValidPoint(point)) {
            points.push_back(point);
        }
    }

    if (points.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("No valid IO points were found in the config file");
    }

    return points;
}

bool IoConfigRepository::save(const QString& filePath, const QVector<IoPoint>& points, QString* errorMessage) const
{
    QJsonObject root;
    bool saveAsLegacy = false;

    QFile existingFile(filePath);
    if (existingFile.exists() && existingFile.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError;
        const QJsonDocument existingDocument = QJsonDocument::fromJson(existingFile.readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError && existingDocument.isObject()) {
            root = existingDocument.object();
            saveAsLegacy = root.contains(QStringLiteral("io_configuration"));
        }
    }

    QJsonArray pointArray;
    for (const IoPoint& point : points) {
        if (!isValidPoint(point)) {
            continue;
        }
        pointArray.push_back(saveAsLegacy ? toLegacyPointObject(point) : toModernPointObject(point));
    }

    if (saveAsLegacy) {
        QJsonObject configObject = root.value(QStringLiteral("io_configuration")).toObject();
        if (configObject.isEmpty()) {
            configObject.insert(QStringLiteral("version"), QStringLiteral("1.0"));
            configObject.insert(QStringLiteral("description"), QStringLiteral("IO Configuration"));
        }
        configObject.insert(QStringLiteral("ios"), pointArray);
        root.insert(QStringLiteral("io_configuration"), configObject);
    } else {
        root = QJsonObject();
        root.insert(QStringLiteral("io_points"), pointArray);
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to save config file: %1").arg(file.errorString());
        }
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

QVector<IoPoint> IoConfigRepository::defaultPoints() const
{
    QVector<IoPoint> points;

    IoPoint di;
    di.id = QStringLiteral("di_0");
    di.variableName = QStringLiteral("general_di_0");
    di.name = QStringLiteral("DI0");
    di.module = QStringLiteral("general");
    di.ioType = QStringLiteral("DI");
    di.direction = QStringLiteral("input");
    di.description = QStringLiteral("Default digital input");
    points.push_back(di);

    IoPoint doPoint;
    doPoint.id = QStringLiteral("do_0");
    doPoint.variableName = QStringLiteral("general_do_0");
    doPoint.name = QStringLiteral("DO0");
    doPoint.module = QStringLiteral("general");
    doPoint.ioType = QStringLiteral("DO");
    doPoint.direction = QStringLiteral("output");
    doPoint.description = QStringLiteral("Default digital output");
    points.push_back(doPoint);

    IoPoint ai;
    ai.id = QStringLiteral("ai_0");
    ai.variableName = QStringLiteral("general_ai_0");
    ai.name = QStringLiteral("AI0");
    ai.module = QStringLiteral("general");
    ai.ioType = QStringLiteral("AI");
    ai.direction = QStringLiteral("input");
    ai.unit = QStringLiteral("V");
    ai.description = QStringLiteral("Default analog input");
    points.push_back(ai);

    IoPoint ao;
    ao.id = QStringLiteral("ao_0");
    ao.variableName = QStringLiteral("general_ao_0");
    ao.name = QStringLiteral("AO0");
    ao.module = QStringLiteral("general");
    ao.ioType = QStringLiteral("AO");
    ao.direction = QStringLiteral("output");
    ao.unit = QStringLiteral("%");
    ao.description = QStringLiteral("Default analog output");
    points.push_back(ao);

    return points;
}

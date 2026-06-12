#include "infrastructure/config/axisperformancerepository.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace
{
AxisPerformanceParameter parseParameter(const QJsonObject& object)
{
    AxisPerformanceParameter parameter;
    parameter.key = object.value(QStringLiteral("key")).toString();
    parameter.label = object.value(QStringLiteral("label")).toString();
    parameter.value = object.value(QStringLiteral("value")).toVariant().toString();
    parameter.unit = object.value(QStringLiteral("unit")).toString();
    parameter.notes = object.value(QStringLiteral("notes")).toString();
    return parameter;
}

AxisPerformanceParameterSet parseParameterSet(const QJsonObject& object)
{
    AxisPerformanceParameterSet parameterSet;
    parameterSet.name = object.value(QStringLiteral("name")).toString();

    const QJsonArray parameterArray = object.value(QStringLiteral("parameters")).toArray();
    for (const QJsonValue& value : parameterArray) {
        if (value.isObject()) {
            parameterSet.parameters.push_back(parseParameter(value.toObject()));
        }
    }

    return parameterSet;
}

AxisPerformanceTestItem parseTestItem(const QJsonObject& object)
{
    AxisPerformanceTestItem item;
    item.key = object.value(QStringLiteral("key")).toString();
    item.name = object.value(QStringLiteral("name")).toString();
    item.description = object.value(QStringLiteral("description")).toString();

    const QJsonArray parameterSetArray = object.value(QStringLiteral("parameterSets")).toArray();
    for (const QJsonValue& value : parameterSetArray) {
        if (value.isObject()) {
            item.parameterSets.push_back(parseParameterSet(value.toObject()));
        }
    }

    const QJsonArray parameterArray = object.value(QStringLiteral("parameters")).toArray();
    for (const QJsonValue& value : parameterArray) {
        if (value.isObject()) {
            item.parameters.push_back(parseParameter(value.toObject()));
        }
    }

    if (item.parameterSets.isEmpty() && item.key == QStringLiteral("settling_time") && !item.parameters.isEmpty()) {
        AxisPerformanceParameterSet parameterSet;
        parameterSet.name = QStringLiteral("Set 1");
        parameterSet.parameters = item.parameters;
        item.parameterSets.push_back(parameterSet);
        item.parameters.clear();
    }

    return item;
}

AxisPerformanceAxis parseAxis(const QJsonObject& object)
{
    AxisPerformanceAxis axis;
    axis.axisName = object.value(QStringLiteral("axisName")).toString();
    axis.axisNumber = object.value(QStringLiteral("axisNumber")).toInt();
    axis.unit = object.value(QStringLiteral("unit")).toString(QStringLiteral("mm"));
    const QJsonArray testArray = object.value(QStringLiteral("testItems")).toArray();
    for (const QJsonValue& value : testArray) {
        if (value.isObject()) {
            axis.testItems.push_back(parseTestItem(value.toObject()));
        }
    }
    return axis;
}
}

QVector<AxisPerformanceProfile> AxisPerformanceRepository::load(const QString& filePath, QString* errorMessage) const
{
    QFile file(filePath);
    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Axis performance config file does not exist: %1").arg(filePath);
        }
        return {};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open axis performance config: %1").arg(file.errorString());
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Axis performance JSON parse failed: %1").arg(parseError.errorString());
        }
        return {};
    }

    QVector<AxisPerformanceProfile> profiles;
    const QJsonArray profileArray = document.object().value(QStringLiteral("axisPerformanceProfiles")).toArray();
    for (const QJsonValue& value : profileArray) {
        if (!value.isObject()) {
            continue;
        }

        const QJsonObject object = value.toObject();
        AxisPerformanceProfile profile;
        profile.profileName = object.value(QStringLiteral("profileName")).toString();
        profile.description = object.value(QStringLiteral("description")).toString();

        const QJsonArray axisArray = object.value(QStringLiteral("axes")).toArray();
        for (const QJsonValue& axisValue : axisArray) {
            if (axisValue.isObject()) {
                profile.axes.push_back(parseAxis(axisValue.toObject()));
            }
        }

        profiles.push_back(profile);
    }

    return profiles;
}

bool AxisPerformanceRepository::save(const QString& filePath,
                                     const QVector<AxisPerformanceProfile>& profiles,
                                     QString* errorMessage) const
{
    QJsonArray profileArray;
    for (const AxisPerformanceProfile& profile : profiles) {
        QJsonObject profileObject;
        profileObject.insert(QStringLiteral("profileName"), profile.profileName);
        profileObject.insert(QStringLiteral("description"), profile.description);

        QJsonArray axisArray;
        for (const AxisPerformanceAxis& axis : profile.axes) {
            QJsonObject axisObject;
            axisObject.insert(QStringLiteral("axisName"), axis.axisName);
            axisObject.insert(QStringLiteral("axisNumber"), axis.axisNumber);
            axisObject.insert(QStringLiteral("unit"), axis.unit);

            QJsonArray testArray;
            for (const AxisPerformanceTestItem& testItem : axis.testItems) {
                QJsonObject testObject;
                testObject.insert(QStringLiteral("key"), testItem.key);
                testObject.insert(QStringLiteral("name"), testItem.name);
                testObject.insert(QStringLiteral("description"), testItem.description);

                auto buildParameterArray = [](const QVector<AxisPerformanceParameter>& parameters) {
                    QJsonArray parameterArray;
                    for (const AxisPerformanceParameter& parameter : parameters) {
                        QJsonObject parameterObject;
                        parameterObject.insert(QStringLiteral("key"), parameter.key);
                        parameterObject.insert(QStringLiteral("label"), parameter.label);
                        parameterObject.insert(QStringLiteral("value"), parameter.value);
                        parameterObject.insert(QStringLiteral("unit"), parameter.unit);
                        if (!parameter.notes.isEmpty()) {
                            parameterObject.insert(QStringLiteral("notes"), parameter.notes);
                        }
                        parameterArray.push_back(parameterObject);
                    }
                    return parameterArray;
                };

                if (!testItem.parameterSets.isEmpty()) {
                    QJsonArray parameterSetArray;
                    for (const AxisPerformanceParameterSet& parameterSet : testItem.parameterSets) {
                        QJsonObject parameterSetObject;
                        parameterSetObject.insert(QStringLiteral("name"), parameterSet.name);
                        parameterSetObject.insert(QStringLiteral("parameters"), buildParameterArray(parameterSet.parameters));
                        parameterSetArray.push_back(parameterSetObject);
                    }
                    testObject.insert(QStringLiteral("parameterSets"), parameterSetArray);
                } else {
                    testObject.insert(QStringLiteral("parameters"), buildParameterArray(testItem.parameters));
                }
                testArray.push_back(testObject);
            }

            axisObject.insert(QStringLiteral("testItems"), testArray);
            axisArray.push_back(axisObject);
        }

        profileObject.insert(QStringLiteral("axes"), axisArray);
        profileArray.push_back(profileObject);
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("axisPerformanceProfiles"), profileArray);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open axis performance config for save: %1").arg(file.errorString());
        }
        return false;
    }

    file.write(QJsonDocument(rootObject).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to save axis performance config: %1").arg(file.errorString());
        }
        return false;
    }

    return true;
}

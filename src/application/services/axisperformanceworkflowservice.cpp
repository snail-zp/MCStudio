#include "application/services/axisperformanceworkflowservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace
{
QString trText(const QString& languageCode, const QString& english, const QString& chinese)
{
    return languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0 ? english : chinese;
}

double maxAbsOf(const QVector<double>& values)
{
    double result = 0.0;
    for (double value : values) {
        if (!std::isfinite(value)) {
            continue;
        }
        result = std::max(result, std::abs(value));
    }
    return result;
}

double scalarFromMeta(const QMap<QString, QString>& meta, const QString& key, double fallback = 0.0)
{
    bool ok = false;
    const double value = meta.value(key).toDouble(&ok);
    return ok ? value : fallback;
}

QString formatMetricValue(double value, const QString& unit, int precision = 4)
{
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', precision), unit).trimmed();
}

double convertUnitValue(double value, const QString& fromUnit, const QString& toUnit)
{
    const QString from = fromUnit.trimmed().toLower();
    const QString to = toUnit.trimmed().toLower();
    if (from.isEmpty() || to.isEmpty() || from == to) {
        return value;
    }
    if (from == QStringLiteral("mm") && to == QStringLiteral("um")) return value * 1000.0;
    if (from == QStringLiteral("um") && to == QStringLiteral("mm")) return value / 1000.0;
    if (from == QStringLiteral("s") && to == QStringLiteral("ms")) return value * 1000.0;
    if (from == QStringLiteral("ms") && to == QStringLiteral("s")) return value / 1000.0;
    return value;
}

const AxisPerformanceSeries* findSeriesByKey(const QVector<AxisPerformanceSeries>& series, const QStringList& keys)
{
    for (const QString& key : keys) {
        const QString lookup = key.trimmed().toLower();
        for (const AxisPerformanceSeries& item : series) {
            const QString candidate = item.key.trimmed().toLower();
            const QString name = item.name.trimmed().toLower();
            if (candidate == lookup || name == lookup || candidate.contains(lookup) || name.contains(lookup)) {
                return &item;
            }
        }
    }
    return nullptr;
}

QString verdictForThreshold(bool passed, const QString& languageCode)
{
    return passed
               ? trText(languageCode, QStringLiteral("PASS"), QStringLiteral("PASS"))
               : trText(languageCode, QStringLiteral("FAIL"), QStringLiteral("FAIL"));
}

void mergeAxisTests(AxisPerformanceImportedAxisResult* targetAxis, const AxisPerformanceImportedAxisResult& sourceAxis)
{
    if (!targetAxis) {
        return;
    }

    if (!sourceAxis.axisName.trimmed().isEmpty()) {
        targetAxis->axisName = sourceAxis.axisName;
    }
    if (sourceAxis.axisNumber != 0 || targetAxis->axisNumber == 0) {
        targetAxis->axisNumber = sourceAxis.axisNumber;
    }

    for (const AxisPerformanceImportedTestResult& sourceTest : sourceAxis.tests) {
        auto existingIt = std::find_if(targetAxis->tests.begin(),
                                       targetAxis->tests.end(),
                                       [&sourceTest](const AxisPerformanceImportedTestResult& existingTest) {
                                           return existingTest.key == sourceTest.key;
                                       });
        if (existingIt != targetAxis->tests.end()) {
            *existingIt = sourceTest;
        } else {
            targetAxis->tests.push_back(sourceTest);
        }
    }
}
}

AxisPerformanceImportedDocument AxisPerformanceWorkflowService::parseResultDocument(const QByteArray& data, QString* errorMessage) const
{
    AxisPerformanceImportedDocument document;

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(data, &parseError);
    if (json.isNull() || !json.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.errorString();
        }
        return document;
    }

    const QJsonObject root = json.object();
    document.sourceFile = root.value(QStringLiteral("sourceFile")).toString();
    const QJsonArray axesArray = root.value(QStringLiteral("axisResults")).toArray().isEmpty()
                                     ? root.value(QStringLiteral("axes")).toArray()
                                     : root.value(QStringLiteral("axisResults")).toArray();

    for (const QJsonValue& axisValue : axesArray) {
        const QVariantMap axisMap = axisValue.toObject().toVariantMap();
        AxisPerformanceImportedAxisResult axisResult;
        axisResult.axisName = axisMap.value(QStringLiteral("axisName")).toString();
        axisResult.axisNumber = axisMap.value(QStringLiteral("axisNumber")).toInt();

        const QVariantList tests = axisMap.value(QStringLiteral("tests")).toList();
        for (const QVariant& testValue : tests) {
            const AxisPerformanceImportedTestResult testResult = parseImportedTest(testValue.toMap());
            if (!testResult.key.isEmpty()) {
                axisResult.tests.push_back(testResult);
            }
        }

        if (!axisResult.axisName.isEmpty() || !axisResult.tests.isEmpty()) {
            document.axes.push_back(axisResult);
        }
    }

    return document;
}

AxisPerformanceImportedTestResult AxisPerformanceWorkflowService::parseImportedTest(const QVariantMap& testMap) const
{
    AxisPerformanceImportedTestResult result;
    result.key = testMap.value(QStringLiteral("key")).toString();
    result.name = testMap.value(QStringLiteral("name")).toString();
    result.description = testMap.value(QStringLiteral("description")).toString();

    const QVariantMap metaMap = testMap.value(QStringLiteral("meta")).toMap();
    for (auto it = metaMap.constBegin(); it != metaMap.constEnd(); ++it) {
        result.meta.insert(it.key(), it.value().toString());
    }

    const QVariant seriesValue = testMap.value(QStringLiteral("series"));
    if (seriesValue.typeId() == QMetaType::QVariantList) {
        const QVariantList seriesList = seriesValue.toList();
        for (const QVariant& entry : seriesList) {
            const QVariantMap seriesMap = entry.toMap();
            AxisPerformanceSeries series;
            series.key = seriesMap.value(QStringLiteral("key")).toString();
            series.name = seriesMap.value(QStringLiteral("name")).toString();
            series.unit = seriesMap.value(QStringLiteral("unit")).toString();
            series.renderStyle = seriesMap.value(QStringLiteral("renderStyle")).toString();
            const QVariantList values = seriesMap.value(QStringLiteral("values")).toList();
            for (const QVariant& value : values) {
                series.values.push_back(value.toDouble());
            }
            if (!series.key.isEmpty() || !series.values.isEmpty()) {
                if (series.key.isEmpty()) {
                    series.key = series.name;
                }
                result.series.push_back(series);
            }
        }
    } else if (seriesValue.typeId() == QMetaType::QVariantMap) {
        const QVariantMap seriesMap = seriesValue.toMap();
        for (auto it = seriesMap.constBegin(); it != seriesMap.constEnd(); ++it) {
            AxisPerformanceSeries series;
            series.key = it.key();
            series.name = it.key();
            const QVariant value = it.value();
            if (value.typeId() == QMetaType::QVariantList) {
                const QVariantList values = value.toList();
                for (const QVariant& item : values) {
                    series.values.push_back(item.toDouble());
                }
            } else {
                series.values.push_back(value.toDouble());
            }
            result.series.push_back(series);
        }
    }

    const QStringList reservedKeys = {QStringLiteral("key"), QStringLiteral("name"), QStringLiteral("description"), QStringLiteral("meta"), QStringLiteral("series")};
    for (auto it = testMap.constBegin(); it != testMap.constEnd(); ++it) {
        if (reservedKeys.contains(it.key())) {
            continue;
        }
        if (it.value().typeId() == QMetaType::QVariantList) {
            AxisPerformanceSeries series;
            series.key = it.key();
            series.name = it.key();
            const QVariantList values = it.value().toList();
            for (const QVariant& value : values) {
                series.values.push_back(value.toDouble());
            }
            result.series.push_back(series);
        } else if (it.value().canConvert<double>()) {
            result.meta.insert(it.key(), it.value().toString());
        }
    }

    return result;
}

void AxisPerformanceWorkflowService::computeMetrics(AxisPerformanceImportedTestResult* testResult,
                                                    const AxisPerformanceAxis* configuredAxis,
                                                    const QString& languageCode) const
{
    if (!testResult) {
        return;
    }

    testResult->metrics.clear();
    const AxisPerformanceTestItem* configuredTest = configuredAxis ? findConfiguredTest(*configuredAxis, testResult->key) : nullptr;
    auto addMetric = [testResult](const QString& name, const QString& value, const QString& status) {
        testResult->metrics.push_back({name, value, status});
    };

    if (testResult->key == QStringLiteral("static_jitter")) {
        const AxisPerformanceSeries* jitterSeries = findSeriesByKey(testResult->series, {QStringLiteral("jitter_per_point"), QStringLiteral("jitter"), QStringLiteral("position")});
        if (jitterSeries && !jitterSeries->values.isEmpty()) {
            const double peakJitter = maxAbsOf(jitterSeries->values);
            const double threshold = parameterValue(configuredTest, QStringLiteral("max_jitter"), 0, 0.0);
            addMetric(QStringLiteral("Peak-to-peak"), formatMetricValue(peakJitter, QStringLiteral("um")), verdictForThreshold(threshold <= 0.0 || peakJitter <= threshold, languageCode));
            testResult->verdict = verdictForThreshold(threshold <= 0.0 || peakJitter <= threshold, languageCode);
        }
    } else if (testResult->key == QStringLiteral("settling_time")) {
        const AxisPerformanceSeries* settleSeries = findSeriesByKey(testResult->series, {QStringLiteral("settle_time_mst"), QStringLiteral("settle_time_delta"), QStringLiteral("settle_time")});
        const double maxTime = parameterValue(configuredTest, QStringLiteral("max_time"), 0, 0.0);

        if (settleSeries && !settleSeries->values.isEmpty()) {
            double maxSettlingTime = -1.0;
            for (double value : settleSeries->values) {
                if (std::isfinite(value)) {
                    maxSettlingTime = std::max(maxSettlingTime, value);
                }
            }
            if (maxSettlingTime >= 0.0) {
                const QString verdict = verdictForThreshold(maxTime <= 0.0 || maxSettlingTime <= maxTime, languageCode);
                addMetric(QStringLiteral("Max Settling Time"), formatMetricValue(maxSettlingTime, QStringLiteral("ms")), verdict);
                testResult->verdict = verdict;
            }
        }
        if (testResult->metrics.isEmpty()) {
            addMetric(QStringLiteral("Settling Time"), trText(languageCode, QStringLiteral("No valid samples"), QString::fromUtf8(u8"无有效采样")), QStringLiteral("-"));
            testResult->verdict = trText(languageCode, QStringLiteral("No Data"), QString::fromUtf8(u8"无数据"));
        }
    } else if (testResult->key == QStringLiteral("speed_uniformity")) {
        const AxisPerformanceSeries* uniformitySeries = findSeriesByKey(testResult->series, {QStringLiteral("uniformity_per_speed_point"), QStringLiteral("uniformity")});
        if (uniformitySeries && !uniformitySeries->values.isEmpty()) {
            double variation = -1.0;
            for (double value : uniformitySeries->values) {
                if (std::isfinite(value)) {
                    variation = std::max(variation, value);
                }
            }
            const double limit = parameterValue(configuredTest, QStringLiteral("max_variation"), 0, 0.0);
            addMetric(QStringLiteral("Uniformity"), formatMetricValue(variation, QStringLiteral("%"), 2), verdictForThreshold(limit <= 0.0 || variation <= limit, languageCode));
            addMetric(QStringLiteral("Speed Points"), QString::number(uniformitySeries->values.size()), QStringLiteral("-"));
            testResult->verdict = verdictForThreshold(limit <= 0.0 || variation <= limit, languageCode);
        }
    } else if (testResult->key == QStringLiteral("dynamic_capability")) {
        const AxisPerformanceSeries* velocitySeries = findSeriesByKey(testResult->series, {QStringLiteral("velocity"), QStringLiteral("vel"), QStringLiteral("fvel")});
        const AxisPerformanceSeries* accelerationSeries = findSeriesByKey(testResult->series, {QStringLiteral("acceleration"), QStringLiteral("acc")});
        const AxisPerformanceSeries* jerkSeries = findSeriesByKey(testResult->series, {QStringLiteral("jerk")});

        if (velocitySeries) {
            const double maxVelocity = maxAbsOf(velocitySeries->values);
            const double limit = parameterValue(configuredTest, QStringLiteral("max_velocity"), 0, 0.0);
            addMetric(QStringLiteral("Max Velocity"), formatMetricValue(maxVelocity, velocitySeries->unit), verdictForThreshold(limit <= 0.0 || maxVelocity >= limit, languageCode));
        }
        if (accelerationSeries) {
            const double maxAcceleration = maxAbsOf(accelerationSeries->values);
            const double limit = parameterValue(configuredTest, QStringLiteral("max_acceleration"), 0, 0.0);
            addMetric(QStringLiteral("Max Acceleration"), formatMetricValue(maxAcceleration, accelerationSeries->unit), verdictForThreshold(limit <= 0.0 || maxAcceleration >= limit, languageCode));
        }
        if (jerkSeries) {
            const double maxJerk = maxAbsOf(jerkSeries->values);
            const double limit = parameterValue(configuredTest, QStringLiteral("max_jerk"), 0, 0.0);
            addMetric(QStringLiteral("Max Jerk"), formatMetricValue(maxJerk, jerkSeries->unit), verdictForThreshold(limit <= 0.0 || maxJerk >= limit, languageCode));
        }
        testResult->verdict = trText(languageCode, QStringLiteral("Computed"), QString::fromUtf8(u8"已计算"));
    } else if (testResult->key == QStringLiteral("power_off_drop_distance")) {
        const AxisPerformanceSeries* dropSeries = findSeriesByKey(testResult->series, {QStringLiteral("drop_distance"), QStringLiteral("distance"), QStringLiteral("drop")});
        if (dropSeries && !dropSeries->values.isEmpty()) {
            const double maxDrop = maxAbsOf(dropSeries->values);
            const double convertedDrop = convertUnitValue(maxDrop, dropSeries->unit, QStringLiteral("um"));
            const double limit = parameterValue(configuredTest, QStringLiteral("max_drop"), 0, 0.0);
            addMetric(QStringLiteral("Max Drop"), formatMetricValue(convertedDrop, QStringLiteral("um")), verdictForThreshold(limit <= 0.0 || convertedDrop <= limit, languageCode));
            testResult->verdict = verdictForThreshold(limit <= 0.0 || convertedDrop <= limit, languageCode);
        }
    } else if (testResult->key == QStringLiteral("limit_window")) {
        const AxisPerformanceSeries* elecNegSeries = findSeriesByKey(testResult->series, {QStringLiteral("electrical_negative"), QStringLiteral("elec_neg")});
        const AxisPerformanceSeries* elecPosSeries = findSeriesByKey(testResult->series, {QStringLiteral("electrical_positive"), QStringLiteral("elec_pos")});
        const AxisPerformanceSeries* softNegSeries = findSeriesByKey(testResult->series, {QStringLiteral("software_negative"), QStringLiteral("soft_neg")});
        const AxisPerformanceSeries* softPosSeries = findSeriesByKey(testResult->series, {QStringLiteral("software_positive"), QStringLiteral("soft_pos")});
        const double elecNeg = elecNegSeries && !elecNegSeries->values.isEmpty() ? elecNegSeries->values.constFirst() : scalarFromMeta(testResult->meta, QStringLiteral("electrical_negative"), scalarFromMeta(testResult->meta, QStringLiteral("elec_neg")));
        const double elecPos = elecPosSeries && !elecPosSeries->values.isEmpty() ? elecPosSeries->values.constFirst() : scalarFromMeta(testResult->meta, QStringLiteral("electrical_positive"), scalarFromMeta(testResult->meta, QStringLiteral("elec_pos")));
        const double softNeg = softNegSeries && !softNegSeries->values.isEmpty() ? softNegSeries->values.constFirst() : scalarFromMeta(testResult->meta, QStringLiteral("software_negative"), scalarFromMeta(testResult->meta, QStringLiteral("soft_neg")));
        const double softPos = softPosSeries && !softPosSeries->values.isEmpty() ? softPosSeries->values.constFirst() : scalarFromMeta(testResult->meta, QStringLiteral("software_positive"), scalarFromMeta(testResult->meta, QStringLiteral("soft_pos")));
        const double stroke = scalarFromMeta(testResult->meta, QStringLiteral("stroke"), softPos - softNeg);
        addMetric(QStringLiteral("Electrical -"), formatMetricValue(elecNeg, QStringLiteral("mm")), QStringLiteral("-"));
        addMetric(QStringLiteral("Electrical +"), formatMetricValue(elecPos, QStringLiteral("mm")), QStringLiteral("-"));
        addMetric(QStringLiteral("Software Stroke"), formatMetricValue(stroke, QStringLiteral("mm")), QStringLiteral("-"));
        testResult->verdict = trText(languageCode, QStringLiteral("Captured"), QString::fromUtf8(u8"已采集"));
    }

    if (testResult->verdict.isEmpty()) {
        testResult->verdict = trText(languageCode, QStringLiteral("Ready"), QString::fromUtf8(u8"就绪"));
    }
}

const AxisPerformanceAxis* AxisPerformanceWorkflowService::findConfiguredAxis(const AxisPerformanceImportedAxisResult& axisResult,
                                                                              const QVector<AxisPerformanceProfile>& profiles) const
{
    for (const AxisPerformanceProfile& profile : profiles) {
        for (const AxisPerformanceAxis& axis : profile.axes) {
            if (axis.axisNumber == axisResult.axisNumber) {
                return &axis;
            }
            if (!axis.axisName.trimmed().isEmpty()
                && axis.axisName.compare(axisResult.axisName, Qt::CaseInsensitive) == 0) {
                return &axis;
            }
        }
    }
    return nullptr;
}

const AxisPerformanceTestItem* AxisPerformanceWorkflowService::findConfiguredTest(const AxisPerformanceAxis& axis, const QString& testKey) const
{
    for (const AxisPerformanceTestItem& testItem : axis.testItems) {
        if (testItem.key == testKey) {
            return &testItem;
        }
    }
    return nullptr;
}

double AxisPerformanceWorkflowService::parameterValue(const AxisPerformanceTestItem* testItem,
                                                      const QString& key,
                                                      int parameterSetIndex,
                                                      double fallback) const
{
    if (!testItem) {
        return fallback;
    }

    const QVector<AxisPerformanceParameter>* parameters = &testItem->parameters;
    if (!testItem->parameterSets.isEmpty()) {
        const int index = qBound(0, parameterSetIndex, testItem->parameterSets.size() - 1);
        parameters = &testItem->parameterSets.at(index).parameters;
    }

    for (const AxisPerformanceParameter& parameter : *parameters) {
        if (parameter.key == key) {
            bool ok = false;
            const double value = parameter.value.toDouble(&ok);
            return ok ? value : fallback;
        }
    }
    return fallback;
}

void AxisPerformanceWorkflowService::recomputeDocumentMetrics(AxisPerformanceImportedDocument* document,
                                                              const QVector<AxisPerformanceProfile>& profiles,
                                                              const QString& languageCode) const
{
    if (!document) {
        return;
    }
    for (AxisPerformanceImportedAxisResult& axisResult : document->axes) {
        const AxisPerformanceAxis* configuredAxis = findConfiguredAxis(axisResult, profiles);
        for (AxisPerformanceImportedTestResult& testResult : axisResult.tests) {
            computeMetrics(&testResult, configuredAxis, languageCode);
        }
    }
}

void AxisPerformanceWorkflowService::mergeResultDocuments(AxisPerformanceImportedDocument* target, const AxisPerformanceImportedDocument& source) const
{
    if (!target) {
        return;
    }

    if (!source.sourceFile.trimmed().isEmpty()) {
        target->sourceFile = source.sourceFile;
    }

    for (const AxisPerformanceImportedAxisResult& sourceAxis : source.axes) {
        auto existingIt = std::find_if(target->axes.begin(),
                                       target->axes.end(),
                                       [&sourceAxis](const AxisPerformanceImportedAxisResult& existingAxis) {
                                           if (sourceAxis.axisNumber != 0 && existingAxis.axisNumber == sourceAxis.axisNumber) {
                                               return true;
                                           }
                                           return !sourceAxis.axisName.trimmed().isEmpty()
                                               && existingAxis.axisName.compare(sourceAxis.axisName, Qt::CaseInsensitive) == 0;
                                       });
        if (existingIt != target->axes.end()) {
            mergeAxisTests(&(*existingIt), sourceAxis);
        } else {
            target->axes.push_back(sourceAxis);
        }
    }
}

QString AxisPerformanceWorkflowService::scriptPathForTestKey(const QString& testKey) const
{
    Q_UNUSED(testKey);
    return QStringLiteral("C:/Users/22841/Desktop/MCStudio/Exports/axis_performance_suite.prg");
}

QString AxisPerformanceWorkflowService::labelForTestKey(const QString& testKey) const
{
    if (testKey == QStringLiteral("limit_window")) return QStringLiteral("AXIS_SOFT_LIMIT_SETUP");
    if (testKey == QStringLiteral("static_jitter")) return QStringLiteral("AXIS_STATIC_JITTER_TEST");
    if (testKey == QStringLiteral("settling_time")) return QStringLiteral("AXIS_SETTLING_TIME_TEST");
    if (testKey == QStringLiteral("speed_uniformity")) return QStringLiteral("AXIS_SPEED_UNIFORMITY_TEST");
    if (testKey == QStringLiteral("dynamic_capability")) return QStringLiteral("AXIS_DYNAMIC_CAPABILITY_TEST");
    if (testKey == QStringLiteral("power_off_drop_distance")) return QStringLiteral("AXIS_DROP_DISTANCE_TEST");
    return QString();
}

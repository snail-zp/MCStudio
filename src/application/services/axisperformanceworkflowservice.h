#pragma once

#include "domain/models/axisperformancetest.h"
#include "domain/models/axisperformanceresult.h"

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVariant>
#include <QVector>

class AxisPerformanceWorkflowService
{
public:
    AxisPerformanceImportedDocument parseResultDocument(const QByteArray& data, QString* errorMessage) const;
    AxisPerformanceImportedTestResult parseImportedTest(const QVariantMap& testMap) const;
    void computeMetrics(AxisPerformanceImportedTestResult* testResult,
                        const AxisPerformanceAxis* configuredAxis,
                        const QString& languageCode) const;
    const AxisPerformanceAxis* findConfiguredAxis(const AxisPerformanceImportedAxisResult& axisResult,
                                                  const QVector<AxisPerformanceProfile>& profiles) const;
    const AxisPerformanceTestItem* findConfiguredTest(const AxisPerformanceAxis& axis, const QString& testKey) const;
    double parameterValue(const AxisPerformanceTestItem* testItem,
                          const QString& key,
                          int parameterSetIndex = 0,
                          double fallback = 0.0) const;
    void recomputeDocumentMetrics(AxisPerformanceImportedDocument* document,
                                  const QVector<AxisPerformanceProfile>& profiles,
                                  const QString& languageCode) const;
    void mergeResultDocuments(AxisPerformanceImportedDocument* target, const AxisPerformanceImportedDocument& source) const;
    QString scriptPathForTestKey(const QString& testKey) const;
    QString labelForTestKey(const QString& testKey) const;
};

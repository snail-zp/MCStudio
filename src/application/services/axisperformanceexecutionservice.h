#pragma once

#include "application/services/controllerservice.h"
#include "domain/models/axisperformancetest.h"
#include "domain/models/axisperformanceresult.h"

class AxisPerformanceExecutionService
{
public:
    explicit AxisPerformanceExecutionService(ControllerService& controllerService);

    bool runControllerBackedTest(const AxisPerformanceAxis& axis,
                                 const AxisPerformanceTestItem& testItem,
                                 AxisPerformanceImportedDocument* document,
                                 QString* errorMessage) const;

private:
    bool prepareControllerBackedTest(const AxisPerformanceAxis& axis,
                                     const AxisPerformanceTestItem& testItem,
                                     QString* doneVariable,
                                     QString* errorVariable,
                                     QString* errorMessage) const;
    bool startSuiteLabel(const QString& labelName, QString* errorMessage) const;
    bool waitForControllerCompletion(const QString& doneVariable,
                                     const QString& errorVariable,
                                     int timeoutMs,
                                     QString* errorMessage) const;
    bool readControllerInteger(const QString& variableName, int* value, QString* errorMessage) const;
    bool writeControllerInteger(const QString& variableName, int value, QString* errorMessage) const;
    bool readControllerReal(const QString& variableName, double* value, QString* errorMessage) const;
    bool writeControllerReal(const QString& variableName, double value, QString* errorMessage) const;
    bool readControllerRealArrayElement(const QString& variableName, int index, double* value, QString* errorMessage) const;
    QVector<double> readControllerRealArray(const QString& variableName, int count, QString* errorMessage) const;
    QVector<double> readControllerRealSeries(const QString& variablePattern, int count, QString* errorMessage) const;
    QVector<double> readControllerRealMatrixRow(const QString& variablePrefix, int rowIndex, int count, QString* errorMessage) const;
    QVector<double> readControllerIntegerMatrixRowAsReal(const QString& variablePrefix, int rowIndex, int count, QString* errorMessage) const;
    AxisPerformanceImportedTestResult buildLiveLimitWindowResult(const AxisPerformanceAxis& axis,
                                                                 const AxisPerformanceTestItem& testItem) const;
    AxisPerformanceImportedTestResult buildLiveStaticJitterResult(const AxisPerformanceAxis& axis,
                                                                  const AxisPerformanceTestItem& testItem,
                                                                  QString* errorMessage) const;
    AxisPerformanceImportedTestResult buildLiveSettlingTimeResult(const AxisPerformanceAxis& axis,
                                                                  const AxisPerformanceTestItem& testItem,
                                                                  QString* errorMessage) const;
    AxisPerformanceImportedTestResult buildLiveSpeedUniformityResult(const AxisPerformanceAxis& axis,
                                                                     const AxisPerformanceTestItem& testItem,
                                                                     QString* errorMessage) const;
    AxisPerformanceImportedTestResult buildLiveDynamicCapabilityResult(const AxisPerformanceAxis& axis,
                                                                       const AxisPerformanceTestItem& testItem,
                                                                       QString* errorMessage) const;
    AxisPerformanceImportedTestResult buildLiveDropDistanceResult(const AxisPerformanceAxis& axis,
                                                                  const AxisPerformanceTestItem& testItem,
                                                                  QString* errorMessage) const;
    double configuredParameterValue(const AxisPerformanceTestItem& testItem, const QString& key, double fallback = 0.0) const;
    QString labelForTestKey(const QString& testKey) const;

    ControllerService& m_controllerService;
};

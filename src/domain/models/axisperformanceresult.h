#pragma once

#include <QMap>
#include <QString>
#include <QVector>

struct AxisPerformanceSeries
{
    QString key;
    QString name;
    QString unit;
    QVector<double> values;
    QString renderStyle;
};

struct AxisPerformanceMetric
{
    QString name;
    QString value;
    QString status;
};

struct AxisPerformanceImportedTestResult
{
    QString key;
    QString name;
    QString description;
    QMap<QString, QString> meta;
    QVector<AxisPerformanceSeries> series;
    QVector<AxisPerformanceMetric> metrics;
    QString verdict;
};

struct AxisPerformanceImportedAxisResult
{
    QString axisName;
    int axisNumber = 0;
    QVector<AxisPerformanceImportedTestResult> tests;
};

struct AxisPerformanceImportedDocument
{
    QString sourceFile;
    QVector<AxisPerformanceImportedAxisResult> axes;
};

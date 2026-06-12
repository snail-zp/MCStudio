#pragma once

#include <QString>
#include <QVector>

struct AxisPerformanceParameter
{
    QString key;
    QString label;
    QString value;
    QString unit;
    QString notes;
};

struct AxisPerformanceParameterSet
{
    QString name;
    QVector<AxisPerformanceParameter> parameters;
};

struct AxisPerformanceTestItem
{
    QString key;
    QString name;
    QString description;
    QVector<AxisPerformanceParameter> parameters;
    QVector<AxisPerformanceParameterSet> parameterSets;
};

struct AxisPerformanceAxis
{
    QString axisName;
    int axisNumber = 0;
    QString unit = QStringLiteral("mm");
    QVector<AxisPerformanceTestItem> testItems;
};

struct AxisPerformanceProfile
{
    QString profileName;
    QString description;
    QVector<AxisPerformanceAxis> axes;
};

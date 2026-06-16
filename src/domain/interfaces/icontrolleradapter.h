#pragma once

#include <QString>
#include <QVector>

#include "domain/models/controllertypes.h"

class IControllerAdapter
{
public:
    virtual ~IControllerAdapter() = default;

    virtual ControllerBrand brand() const = 0;
    virtual bool connect(const ControllerConnectionOptions& options, QString* errorMessage) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual bool readIntegerVariable(const QString& variableName, int* value, QString* errorMessage, int bufferNumber) const = 0;
    virtual bool writeIntegerVariable(const QString& variableName, int value, QString* errorMessage, int bufferNumber) const = 0;
    virtual bool readRealVariable(const QString& variableName, double* value, QString* errorMessage) const = 0;
    virtual bool writeRealVariable(const QString& variableName, double value, QString* errorMessage) const = 0;
    virtual bool readIntegerArray(const QString& variableName,
                                  int fromIndex,
                                  int toIndex,
                                  QVector<int>* values,
                                  QString* errorMessage) const = 0;
    virtual bool readIntegerMatrixRow(const QString& variableName,
                                      int rowIndex,
                                      int fromColumn,
                                      int toColumn,
                                      QVector<int>* values,
                                      QString* errorMessage) const = 0;
    virtual bool readRealArray(const QString& variableName,
                               int fromIndex,
                               int toIndex,
                               QVector<double>* values,
                               QString* errorMessage) const = 0;
    virtual bool readRealMatrixRow(const QString& variableName,
                                   int rowIndex,
                                   int fromColumn,
                                   int toColumn,
                                   QVector<double>* values,
                                   QString* errorMessage) const = 0;
    virtual bool executeCommand(const QString& commandText, QString* response, QString* errorMessage) = 0;
    virtual bool isProgramBufferRunning(int bufferNumber, bool* running, QString* errorMessage) const = 0;
    virtual bool moveAxisRelative(int axisNumber, double distance, double velocity, QString* errorMessage) = 0;
    virtual bool moveAxisToPosition(int axisNumber, double targetPosition, double velocity, QString* errorMessage) = 0;
    virtual bool haltAxis(int axisNumber, QString* errorMessage) = 0;
    virtual bool readAxisPosition(int axisNumber, double* position, QString* errorMessage) const = 0;
    virtual bool exportHardwareCode(const QString& directoryPath, QString* errorMessage) = 0;
};

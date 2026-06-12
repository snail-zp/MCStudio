#pragma once

#include <memory>

#include <QString>
#include <QVector>

#include "domain/models/controllertypes.h"

class ControllerService
{
public:
    using Brand = ControllerBrand;
    using ConnectionType = ControllerConnectionType;

    ControllerService();
    ~ControllerService();

    void setControllerBrand(Brand brand);
    Brand controllerBrand() const;

    bool connect(ConnectionType type, const QString& address, int port, QString* errorMessage);
    void disconnect();
    bool isConnected() const;

    bool readIntegerVariable(const QString& variableName, int* value, QString* errorMessage) const;
    bool readIntegerVariable(const QString& variableName, int* value, QString* errorMessage, int bufferNumber) const;
    bool writeIntegerVariable(const QString& variableName, int value, QString* errorMessage) const;
    bool writeIntegerVariable(const QString& variableName, int value, QString* errorMessage, int bufferNumber) const;
    bool readRealVariable(const QString& variableName, double* value, QString* errorMessage) const;
    bool writeRealVariable(const QString& variableName, double value, QString* errorMessage) const;
    bool readIntegerArray(const QString& variableName,
                          int fromIndex,
                          int toIndex,
                          QVector<int>* values,
                          QString* errorMessage) const;
    bool readIntegerMatrixRow(const QString& variableName,
                              int rowIndex,
                              int fromColumn,
                              int toColumn,
                              QVector<int>* values,
                              QString* errorMessage) const;
    bool readRealArray(const QString& variableName,
                       int fromIndex,
                       int toIndex,
                       QVector<double>* values,
                       QString* errorMessage) const;
    bool readRealMatrixRow(const QString& variableName,
                           int rowIndex,
                           int fromColumn,
                           int toColumn,
                           QVector<double>* values,
                           QString* errorMessage) const;
    bool executeCommand(const QString& commandText, QString* response, QString* errorMessage);
    bool moveAxisRelative(int axisNumber, double distance, double velocity, QString* errorMessage);
    bool moveAxisToPosition(int axisNumber, double targetPosition, double velocity, QString* errorMessage);
    bool haltAxis(int axisNumber, QString* errorMessage);
    bool readAxisPosition(int axisNumber, double* position, QString* errorMessage) const;
    bool exportHardwareCode(const QString& directoryPath, QString* errorMessage);

private:
    void ensureControllerAdapter();

    Brand m_brand = Brand::Acs;
    std::unique_ptr<class IControllerAdapter> m_controllerAdapter;
};

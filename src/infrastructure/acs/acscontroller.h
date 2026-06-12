#pragma once

#include <QString>
#include <QVector>
#include <windows.h>

#include "domain/interfaces/icontrolleradapter.h"

class AcsController : public IControllerAdapter
{
public:
    AcsController();
    ~AcsController() override;

    ControllerBrand brand() const override;
    bool connect(const ControllerConnectionOptions& options, QString* errorMessage) override;
    void disconnect() override;

    bool isConnected() const override;
    bool readIntegerVariable(const QString& variableName, int* value, QString* errorMessage, int bufferNumber) const override;
    bool writeIntegerVariable(const QString& variableName, int value, QString* errorMessage, int bufferNumber) const override;
    bool readRealVariable(const QString& variableName, double* value, QString* errorMessage) const override;
    bool writeRealVariable(const QString& variableName, double value, QString* errorMessage) const override;
    bool readIntegerArray(const QString& variableName,
                          int fromIndex,
                          int toIndex,
                          QVector<int>* values,
                          QString* errorMessage) const override;
    bool readIntegerMatrixRow(const QString& variableName,
                              int rowIndex,
                              int fromColumn,
                              int toColumn,
                              QVector<int>* values,
                              QString* errorMessage) const override;
    bool readRealArray(const QString& variableName,
                       int fromIndex,
                       int toIndex,
                       QVector<double>* values,
                       QString* errorMessage) const override;
    bool readRealMatrixRow(const QString& variableName,
                           int rowIndex,
                           int fromColumn,
                           int toColumn,
                           QVector<double>* values,
                           QString* errorMessage) const override;
    bool executeCommand(const QString& commandText, QString* response, QString* errorMessage) override;
    bool moveAxisRelative(int axisNumber, double distance, double velocity, QString* errorMessage) override;
    bool moveAxisToPosition(int axisNumber, double targetPosition, double velocity, QString* errorMessage) override;
    bool haltAxis(int axisNumber, QString* errorMessage) override;
    bool readAxisPosition(int axisNumber, double* position, QString* errorMessage) const override;
    bool exportHardwareCode(const QString& directoryPath, QString* errorMessage) override;

private:
    bool ensureAxisEnabled(int axisNumber, QString* errorMessage) const;
    QString buildErrorMessage(const QString& action) const;

    HANDLE m_handle;
};

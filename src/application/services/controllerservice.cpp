#include "application/services/controllerservice.h"

#include "domain/interfaces/icontrolleradapter.h"
#include "infrastructure/controllers/controlleradapterfactory.h"

ControllerService::ControllerService() = default;

ControllerService::~ControllerService() = default;

void ControllerService::setControllerBrand(Brand brand)
{
    if (m_brand == brand && m_controllerAdapter) {
        return;
    }

    if (m_controllerAdapter) {
        m_controllerAdapter->disconnect();
    }

    m_brand = brand;
    m_controllerAdapter.reset();
}

ControllerService::Brand ControllerService::controllerBrand() const
{
    return m_brand;
}

bool ControllerService::connect(ConnectionType type, const QString& address, int port, QString* errorMessage)
{
    ensureControllerAdapter();

    ControllerConnectionOptions options;
    options.connectionType = type;
    options.address = address;
    options.port = port;
    return m_controllerAdapter->connect(options, errorMessage);
}

void ControllerService::disconnect()
{
    if (m_controllerAdapter) {
        m_controllerAdapter->disconnect();
    }
}

bool ControllerService::isConnected() const
{
    return m_controllerAdapter && m_controllerAdapter->isConnected();
}

bool ControllerService::readIntegerVariable(const QString& variableName, int* value, QString* errorMessage) const
{
    return readIntegerVariable(variableName, value, errorMessage, -99999);
}

bool ControllerService::readIntegerVariable(const QString& variableName, int* value, QString* errorMessage, int bufferNumber) const
{
    return m_controllerAdapter && m_controllerAdapter->readIntegerVariable(variableName, value, errorMessage, bufferNumber);
}

bool ControllerService::writeIntegerVariable(const QString& variableName, int value, QString* errorMessage) const
{
    return writeIntegerVariable(variableName, value, errorMessage, -99999);
}

bool ControllerService::writeIntegerVariable(const QString& variableName, int value, QString* errorMessage, int bufferNumber) const
{
    return m_controllerAdapter && m_controllerAdapter->writeIntegerVariable(variableName, value, errorMessage, bufferNumber);
}

bool ControllerService::readRealVariable(const QString& variableName, double* value, QString* errorMessage) const
{
    return m_controllerAdapter && m_controllerAdapter->readRealVariable(variableName, value, errorMessage);
}

bool ControllerService::writeRealVariable(const QString& variableName, double value, QString* errorMessage) const
{
    return m_controllerAdapter && m_controllerAdapter->writeRealVariable(variableName, value, errorMessage);
}

bool ControllerService::readIntegerArray(const QString& variableName,
                                         int fromIndex,
                                         int toIndex,
                                         QVector<int>* values,
                                         QString* errorMessage) const
{
    return m_controllerAdapter && m_controllerAdapter->readIntegerArray(variableName, fromIndex, toIndex, values, errorMessage);
}

bool ControllerService::readIntegerMatrixRow(const QString& variableName,
                                             int rowIndex,
                                             int fromColumn,
                                             int toColumn,
                                             QVector<int>* values,
                                             QString* errorMessage) const
{
    return m_controllerAdapter
        && m_controllerAdapter->readIntegerMatrixRow(variableName, rowIndex, fromColumn, toColumn, values, errorMessage);
}

bool ControllerService::readRealArray(const QString& variableName,
                                      int fromIndex,
                                      int toIndex,
                                      QVector<double>* values,
                                      QString* errorMessage) const
{
    return m_controllerAdapter && m_controllerAdapter->readRealArray(variableName, fromIndex, toIndex, values, errorMessage);
}

bool ControllerService::readRealMatrixRow(const QString& variableName,
                                          int rowIndex,
                                          int fromColumn,
                                          int toColumn,
                                          QVector<double>* values,
                                          QString* errorMessage) const
{
    return m_controllerAdapter
        && m_controllerAdapter->readRealMatrixRow(variableName, rowIndex, fromColumn, toColumn, values, errorMessage);
}

bool ControllerService::executeCommand(const QString& commandText, QString* response, QString* errorMessage)
{
    return m_controllerAdapter && m_controllerAdapter->executeCommand(commandText, response, errorMessage);
}

bool ControllerService::isProgramBufferRunning(int bufferNumber, bool* running, QString* errorMessage) const
{
    return m_controllerAdapter && m_controllerAdapter->isProgramBufferRunning(bufferNumber, running, errorMessage);
}

bool ControllerService::moveAxisRelative(int axisNumber, double distance, double velocity, QString* errorMessage)
{
    return m_controllerAdapter && m_controllerAdapter->moveAxisRelative(axisNumber, distance, velocity, errorMessage);
}

bool ControllerService::moveAxisToPosition(int axisNumber, double targetPosition, double velocity, QString* errorMessage)
{
    return m_controllerAdapter && m_controllerAdapter->moveAxisToPosition(axisNumber, targetPosition, velocity, errorMessage);
}

bool ControllerService::haltAxis(int axisNumber, QString* errorMessage)
{
    return m_controllerAdapter && m_controllerAdapter->haltAxis(axisNumber, errorMessage);
}

bool ControllerService::readAxisPosition(int axisNumber, double* position, QString* errorMessage) const
{
    return m_controllerAdapter && m_controllerAdapter->readAxisPosition(axisNumber, position, errorMessage);
}

bool ControllerService::exportHardwareCode(const QString& directoryPath, QString* errorMessage)
{
    return m_controllerAdapter && m_controllerAdapter->exportHardwareCode(directoryPath, errorMessage);
}

void ControllerService::ensureControllerAdapter()
{
    if (!m_controllerAdapter) {
        m_controllerAdapter = ControllerAdapterFactory::create(m_brand);
    }
}

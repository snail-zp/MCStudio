#include "infrastructure/acs/acscontroller.h"

#include <cstring>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtMath>

#include "ACSC.h"

AcsController::AcsController()
    : m_handle(ACSC_INVALID)
{
}

AcsController::~AcsController()
{
    disconnect();
}

ControllerBrand AcsController::brand() const
{
    return ControllerBrand::Acs;
}

bool AcsController::connect(const ControllerConnectionOptions& options, QString* errorMessage)
{
    disconnect();

    switch (options.connectionType) {
    case ControllerConnectionType::Simulator:
        m_handle = acsc_OpenCommSimulator();
        break;
    case ControllerConnectionType::EthernetTcp: {
        const QByteArray ipBytes = options.address.toLocal8Bit();
        m_handle = acsc_OpenCommEthernetTCP(const_cast<char*>(ipBytes.data()), options.port);
        break;
    }
    case ControllerConnectionType::EthernetUdp: {
        const QByteArray ipBytes = options.address.toLocal8Bit();
        m_handle = acsc_OpenCommEthernetUDP(const_cast<char*>(ipBytes.data()), options.port);
        break;
    }
    }

    if (m_handle == ACSC_INVALID) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to connect ACS controller"));
        }
        return false;
    }

    return true;
}

void AcsController::disconnect()
{
    if (m_handle != ACSC_INVALID) {
        acsc_CloseComm(m_handle);
        m_handle = ACSC_INVALID;
    }
}

bool AcsController::isConnected() const
{
    return m_handle != ACSC_INVALID;
}

bool AcsController::readIntegerVariable(const QString& variableName, int* value, QString* errorMessage, int bufferNumber) const
{
    QByteArray bytes = variableName.toLocal8Bit();
    const int targetBuffer = (bufferNumber == -99999) ? ACSC_NONE : bufferNumber;
    if (!acsc_ReadInteger(m_handle, targetBuffer, bytes.data(), ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, value, nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read integer variable %1 from buffer %2").arg(variableName).arg(targetBuffer));
        }
        return false;
    }
    return true;
}

bool AcsController::writeIntegerVariable(const QString& variableName, int value, QString* errorMessage, int bufferNumber) const
{
    QByteArray bytes = variableName.toLocal8Bit();
    int writeValue = value;
    const int targetBuffer = (bufferNumber == -99999) ? ACSC_NONE : bufferNumber;
    if (!acsc_WriteInteger(m_handle, targetBuffer, bytes.data(), ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, &writeValue, nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to write integer variable %1 to buffer %2").arg(variableName).arg(targetBuffer));
        }
        return false;
    }
    return true;
}

bool AcsController::readRealVariable(const QString& variableName, double* value, QString* errorMessage) const
{
    QByteArray bytes = variableName.toLocal8Bit();
    if (!acsc_ReadReal(m_handle, ACSC_NONE, bytes.data(), ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, value, nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read real variable %1").arg(variableName));
        }
        return false;
    }
    return true;
}

bool AcsController::writeRealVariable(const QString& variableName, double value, QString* errorMessage) const
{
    QByteArray bytes = variableName.toLocal8Bit();
    double writeValue = value;
    if (!acsc_WriteReal(m_handle, ACSC_NONE, bytes.data(), ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, &writeValue, nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to write real variable %1").arg(variableName));
        }
        return false;
    }
    return true;
}

bool AcsController::readIntegerArray(const QString& variableName,
                                     int fromIndex,
                                     int toIndex,
                                     QVector<int>* values,
                                     QString* errorMessage) const
{
    if (!values) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Integer array output buffer is null");
        }
        return false;
    }
    if (toIndex < fromIndex) {
        values->clear();
        return true;
    }

    values->resize(toIndex - fromIndex + 1);
    QByteArray bytes = variableName.toLocal8Bit();
    if (!acsc_ReadInteger(m_handle,
                          ACSC_NONE,
                          bytes.data(),
                          fromIndex,
                          toIndex,
                          ACSC_NONE,
                          ACSC_NONE,
                          values->data(),
                          nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read integer array %1").arg(variableName));
        }
        values->clear();
        return false;
    }
    return true;
}

bool AcsController::readIntegerMatrixRow(const QString& variableName,
                                         int rowIndex,
                                         int fromColumn,
                                         int toColumn,
                                         QVector<int>* values,
                                         QString* errorMessage) const
{
    if (!values) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Integer matrix output buffer is null");
        }
        return false;
    }
    if (toColumn < fromColumn) {
        values->clear();
        return true;
    }

    values->resize(toColumn - fromColumn + 1);
    QByteArray bytes = variableName.toLocal8Bit();
    if (!acsc_ReadInteger(m_handle,
                          ACSC_NONE,
                          bytes.data(),
                          rowIndex,
                          rowIndex,
                          fromColumn,
                          toColumn,
                          values->data(),
                          nullptr)
        && !acsc_ReadInteger(m_handle,
                             ACSC_NONE,
                             bytes.data(),
                             fromColumn,
                             toColumn,
                             rowIndex,
                             rowIndex,
                             values->data(),
                             nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read integer matrix row %1(%2)").arg(variableName).arg(rowIndex));
        }
        values->clear();
        return false;
    }
    return true;
}

bool AcsController::readRealArray(const QString& variableName,
                                  int fromIndex,
                                  int toIndex,
                                  QVector<double>* values,
                                  QString* errorMessage) const
{
    if (!values) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Real array output buffer is null");
        }
        return false;
    }
    if (toIndex < fromIndex) {
        values->clear();
        return true;
    }

    values->resize(toIndex - fromIndex + 1);
    QByteArray bytes = variableName.toLocal8Bit();
    if (!acsc_ReadReal(m_handle,
                       ACSC_NONE,
                       bytes.data(),
                       fromIndex,
                       toIndex,
                       ACSC_NONE,
                       ACSC_NONE,
                       values->data(),
                       nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read real array %1").arg(variableName));
        }
        values->clear();
        return false;
    }
    return true;
}

bool AcsController::readRealMatrixRow(const QString& variableName,
                                      int rowIndex,
                                      int fromColumn,
                                      int toColumn,
                                      QVector<double>* values,
                                      QString* errorMessage) const
{
    if (!values) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Real matrix output buffer is null");
        }
        return false;
    }
    if (toColumn < fromColumn) {
        values->clear();
        return true;
    }

    values->resize(toColumn - fromColumn + 1);
    QByteArray bytes = variableName.toLocal8Bit();
    if (!acsc_ReadReal(m_handle,
                       ACSC_NONE,
                       bytes.data(),
                       rowIndex,
                       rowIndex,
                       fromColumn,
                       toColumn,
                       values->data(),
                       nullptr)
        && !acsc_ReadReal(m_handle,
                          ACSC_NONE,
                          bytes.data(),
                          fromColumn,
                          toColumn,
                          rowIndex,
                          rowIndex,
                          values->data(),
                          nullptr)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read real matrix row %1(%2)").arg(variableName).arg(rowIndex));
        }
        values->clear();
        return false;
    }
    return true;
}

bool AcsController::executeCommand(const QString& commandText, QString* response, QString* errorMessage)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ACS controller is not connected");
        }
        return false;
    }

    QByteArray buffer(1024, '\0');
    const QByteArray commandBytes = commandText.toLocal8Bit();
    const int copyLength = qMin(commandBytes.size(), buffer.size() - 1);
    memcpy(buffer.data(), commandBytes.constData(), copyLength);
    if (!acsc_Command(m_handle, buffer.data(), buffer.size(), ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to execute ACS command"));
        }
        return false;
    }

    if (response) {
        *response = QString::fromLocal8Bit(buffer.constData());
    }
    return true;
}

bool AcsController::moveAxisRelative(int axisNumber, double distance, double velocity, QString* errorMessage)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ACS controller is not connected");
        }
        return false;
    }

    if (!ensureAxisEnabled(axisNumber, errorMessage)) {
        return false;
    }

    const double absoluteVelocity = qAbs(velocity);
    if (!acsc_SetVelocity(m_handle, axisNumber, absoluteVelocity, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to set axis %1 velocity").arg(axisNumber));
        }
        return false;
    }

    double currentPosition = 0.0;
    if (!acsc_GetRPosition(m_handle, axisNumber, &currentPosition, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read axis %1 position").arg(axisNumber));
        }
        return false;
    }

    const double targetPosition = currentPosition + distance;
    if (!acsc_ToPoint(m_handle, 0, axisNumber, targetPosition, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to move axis %1 to target").arg(axisNumber));
        }
        return false;
    }

    return true;
}

bool AcsController::moveAxisToPosition(int axisNumber, double targetPosition, double velocity, QString* errorMessage)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ACS controller is not connected");
        }
        return false;
    }

    if (!ensureAxisEnabled(axisNumber, errorMessage)) {
        return false;
    }

    const double absoluteVelocity = qAbs(velocity);
    if (!acsc_SetVelocity(m_handle, axisNumber, absoluteVelocity, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to set axis %1 velocity").arg(axisNumber));
        }
        return false;
    }

    if (!acsc_ToPoint(m_handle, 0, axisNumber, targetPosition, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to move axis %1 to target position").arg(axisNumber));
        }
        return false;
    }

    return true;
}

bool AcsController::haltAxis(int axisNumber, QString* errorMessage)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ACS controller is not connected");
        }
        return false;
    }

    if (!acsc_Halt(m_handle, axisNumber, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to halt axis %1").arg(axisNumber));
        }
        return false;
    }

    return true;
}

bool AcsController::readAxisPosition(int axisNumber, double* position, QString* errorMessage) const
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ACS controller is not connected");
        }
        return false;
    }

    if (!position) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Axis position output buffer is null");
        }
        return false;
    }

    if (!acsc_GetRPosition(m_handle, axisNumber, position, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read axis %1 position").arg(axisNumber));
        }
        return false;
    }

    return true;
}

bool AcsController::exportHardwareCode(const QString& directoryPath, QString* errorMessage)
{
    if (!isConnected()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ACS controller is not connected");
        }
        return false;
    }

    QFileInfo outputInfo(directoryPath);
    const QString outputFilePath = outputInfo.suffix().compare(QStringLiteral("prg"), Qt::CaseInsensitive) == 0
        ? directoryPath
        : QDir(directoryPath).filePath(QStringLiteral("hardware_code.prg"));
    QDir outputDir(QFileInfo(outputFilePath).absolutePath());
    if (!outputDir.exists() && !outputDir.mkpath(QStringLiteral("."))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to create export directory: %1").arg(outputDir.absolutePath());
        }
        return false;
    }

    double bufferCount = 0.0;
    if (!acsc_GetBuffersCount(m_handle, &bufferCount, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to get ACS buffer count"));
        }
        return false;
    }

    QByteArray mergedContent;
    int exportedCount = 0;
    for (int bufferIndex = 0; bufferIndex < static_cast<int>(bufferCount); ++bufferIndex) {
        QByteArray buffer(65536, '\0');
        int received = 0;
        if (!acsc_UploadBuffer(m_handle, bufferIndex, 0, buffer.data(), buffer.size() - 1, &received, ACSC_SYNCHRONOUS)) {
            continue;
        }

        if (received <= 0) {
            continue;
        }
        mergedContent.append(QByteArray("/* ===== BUFFER "));
        mergedContent.append(QByteArray::number(bufferIndex));
        mergedContent.append(QByteArray(" ===== */\r\n"));
        mergedContent.append(buffer.constData(), received);
        if (!mergedContent.endsWith("\n")) {
            mergedContent.append("\r\n");
        }
        mergedContent.append("\r\n");
        ++exportedCount;
    }

    if (exportedCount == 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No ACS program buffers were exported");
        }
        return false;
    }

    QFile file(outputFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to write hardware code file: %1").arg(outputFilePath);
        }
        return false;
    }
    file.write(mergedContent);
    file.close();

    return true;
}

bool AcsController::ensureAxisEnabled(int axisNumber, QString* errorMessage) const
{
    int motorState = 0;
    if (!acsc_GetMotorState(m_handle, axisNumber, &motorState, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to read axis %1 state").arg(axisNumber));
        }
        return false;
    }

    if ((motorState & ACSC_MST_ENABLE) != 0) {
        return true;
    }

    if (!acsc_Enable(m_handle, axisNumber, ACSC_SYNCHRONOUS)) {
        if (errorMessage) {
            *errorMessage = buildErrorMessage(QStringLiteral("Failed to enable axis %1").arg(axisNumber));
        }
        return false;
    }

    return true;
}

QString AcsController::buildErrorMessage(const QString& action) const
{
    const int errorCode = acsc_GetLastError();
    char buffer[512] = {};
    int received = 0;

    if (m_handle != ACSC_INVALID && acsc_GetErrorString(m_handle, errorCode, buffer, 511, &received) && received > 0) {
        return QStringLiteral("%1: [%2] %3")
            .arg(action)
            .arg(errorCode)
            .arg(QString::fromLocal8Bit(buffer, received));
    }

    return QStringLiteral("%1: error code %2").arg(action).arg(errorCode);
}

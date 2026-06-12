#include "presentation/axisperformancepage.h"
#include <QAbstractItemView>
#include "presentation/uihelpers.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QBuffer>
#include <QDoubleValidator>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QStringConverter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTextStream>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
struct AxisPerformanceRunOutcome
{
    bool ok = false;
    int axisNumber = 0;
    QString testKey;
    QString testDisplayName;
    AxisPerformanceImportedDocument liveDocument;
    QString errorMessage;
};

class AxisPerformanceChartWidget : public QWidget
{
public:
    explicit AxisPerformanceChartWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(260);
    }

    void setSeries(const QVector<AxisPerformanceSeries>& series)
    {
        m_series = series;
        update();
    }

    void setXAxisInfo(const QString& title, const QString& leftLabel, const QString& centerLabel, const QString& rightLabel)
    {
        m_xAxisTitle = title;
        m_xAxisLeftLabel = leftLabel;
        m_xAxisCenterLabel = centerLabel;
        m_xAxisRightLabel = rightLabel;
        update();
    }

    void setYAxisInfo(const QString& title)
    {
        m_yAxisTitle = title;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(QStringLiteral("#ffffff")));

        const QRect frame = rect().adjusted(10, 10, -10, -10);
        painter.setPen(QColor(QStringLiteral("#d8e0e8")));
        painter.drawRoundedRect(frame, 10, 10);

        if (m_series.isEmpty()) {
            painter.setPen(QColor(QStringLiteral("#7b8a97")));
            painter.drawText(frame, Qt::AlignCenter, QStringLiteral("No data"));
            return;
        }

        double minValue = 0.0;
        double maxValue = 0.0;
        int maxCount = 0;
        bool hasValue = false;
        for (const AxisPerformanceSeries& series : m_series) {
            maxCount = std::max(maxCount, static_cast<int>(series.values.size()));
            for (double value : series.values) {
                if (!std::isfinite(value)) {
                    continue;
                }
                if (!hasValue) {
                    minValue = maxValue = value;
                    hasValue = true;
                } else {
                    minValue = std::min(minValue, value);
                    maxValue = std::max(maxValue, value);
                }
            }
        }

        if (!hasValue || maxCount <= 1) {
            painter.setPen(QColor(QStringLiteral("#7b8a97")));
            painter.drawText(frame, Qt::AlignCenter, QStringLiteral("Not enough samples"));
            return;
        }

        if (std::abs(maxValue - minValue) < 1e-9) {
            maxValue += 1.0;
            minValue -= 1.0;
        }

        const QVector<QColor> palette = {
            QColor(QStringLiteral("#0f766e")),
            QColor(QStringLiteral("#2563eb")),
            QColor(QStringLiteral("#d97706")),
            QColor(QStringLiteral("#dc2626")),
            QColor(QStringLiteral("#7c3aed"))
        };

        struct LegendEntry
        {
            QString text;
            QColor color;
            int width = 0;
        };

        QVector<LegendEntry> legends;
        legends.reserve(m_series.size());
        QFontMetrics metrics(font());
        for (int index = 0; index < m_series.size(); ++index) {
            const AxisPerformanceSeries& series = m_series.at(index);
            QColor color = palette.at(index % palette.size());
            if (series.renderStyle == QStringLiteral("range_highlight")) {
                color = QColor(QStringLiteral("#d97706"));
            } else if (series.renderStyle == QStringLiteral("marker")) {
                color = QColor(QStringLiteral("#dc2626"));
            }
            LegendEntry entry;
            entry.text = series.name.isEmpty() ? series.key : series.name;
            entry.color = color;
            entry.width = 24 + metrics.horizontalAdvance(entry.text);
            legends.push_back(entry);
        }

        const int legendLeft = frame.left() + 8;
        const int legendTop = frame.bottom() - 18;
        const int legendAvailableWidth = qMax(frame.width() - 20, 120);
        int legendRows = legends.isEmpty() ? 0 : 1;
        int legendCursor = 0;
        for (const LegendEntry& entry : legends) {
            if (legendCursor > 0 && (legendCursor + entry.width) > legendAvailableWidth) {
                legendRows += 1;
                legendCursor = 0;
            }
            legendCursor += entry.width + 14;
        }
        const int legendHeight = legendRows > 0 ? (legendRows * 18) + 8 : 0;
        const QRect plotRect = frame.adjusted(68, 18, -18, -(34 + legendHeight));
        painter.setPen(QColor(QStringLiteral("#eef2f6")));
        for (int row = 0; row < 5; ++row) {
            const int y = plotRect.top() + row * plotRect.height() / 4;
            painter.drawLine(plotRect.left(), y, plotRect.right(), y);
        }

        painter.setPen(QColor(QStringLiteral("#a8b4bf")));
        painter.drawLine(plotRect.bottomLeft(), plotRect.bottomRight());
        painter.drawLine(plotRect.bottomLeft(), plotRect.topLeft());

        int colorIndex = 0;
        painter.save();
        painter.setClipRect(plotRect.adjusted(-4, -4, 4, 4));
        for (const AxisPerformanceSeries& series : m_series) {
            QVector<QPointF> points;
            points.reserve(series.values.size());
            for (int i = 0; i < series.values.size(); ++i) {
                const double value = series.values.at(i);
                if (!std::isfinite(value)) {
                    continue;
                }
                const double xRatio = maxCount <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(maxCount - 1);
                const double yRatio = (value - minValue) / (maxValue - minValue);
                const double x = plotRect.left() + xRatio * plotRect.width();
                const double y = plotRect.bottom() - yRatio * plotRect.height();
                points.push_back(QPointF(x, y));
            }
            if (points.isEmpty()) {
                continue;
            }
            QColor color = palette.at(colorIndex % palette.size());
            qreal penWidth = 2.0;
            if (series.renderStyle == QStringLiteral("range_highlight")) {
                color = QColor(QStringLiteral("#d97706"));
                penWidth = 4.0;
            } else if (series.renderStyle == QStringLiteral("marker")) {
                color = QColor(QStringLiteral("#dc2626"));
            }
            painter.setPen(QPen(color, penWidth));
            for (int i = 1; i < points.size(); ++i) {
                painter.drawLine(points.at(i - 1), points.at(i));
            }
            painter.setBrush(color);
            const qreal pointRadius = series.renderStyle == QStringLiteral("marker") ? 4.0 : 2.6;
            for (const QPointF& point : points) {
                painter.drawEllipse(point, pointRadius, pointRadius);
            }
            ++colorIndex;
        }
        painter.restore();

        int legendX = legendLeft;
        int legendY = plotRect.bottom() + 28;
        for (const LegendEntry& entry : legends) {
            if (legendX > legendLeft && (legendX + entry.width) > (legendLeft + legendAvailableWidth)) {
                legendX = legendLeft;
                legendY += 18;
            }
            painter.fillRect(QRect(legendX, legendY - 8, 12, 12), entry.color);
            painter.setPen(QColor(QStringLiteral("#31424f")));
            painter.drawText(legendX + 16, legendY + 2, entry.text);
            legendX += entry.width + 14;
        }

        painter.setPen(QColor(QStringLiteral("#526372")));
        painter.drawText(QRect(frame.left() + 4, plotRect.top() - 8, 36, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(maxValue, 'f', 3));
        painter.drawText(QRect(frame.left() + 4, plotRect.bottom() - 8, 36, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(minValue, 'f', 3));

        painter.setPen(QColor(QStringLiteral("#526372")));
        painter.drawText(QRect(plotRect.left() - 10, plotRect.bottom() + 6, 80, 16),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         m_xAxisLeftLabel);
        painter.drawText(QRect(plotRect.center().x() - 50, plotRect.bottom() + 6, 100, 16),
                         Qt::AlignCenter | Qt::AlignVCenter,
                         m_xAxisCenterLabel);
        painter.drawText(QRect(plotRect.right() - 70, plotRect.bottom() + 6, 80, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         m_xAxisRightLabel);
        painter.drawText(QRect(plotRect.left(), frame.bottom() - 22, plotRect.width(), 18),
                         Qt::AlignCenter | Qt::AlignVCenter,
                         m_xAxisTitle);
        if (!m_yAxisTitle.isEmpty()) {
            painter.save();
            painter.setPen(QColor(QStringLiteral("#526372")));
            painter.translate(frame.left() + 18, plotRect.center().y());
            painter.rotate(-90);
            painter.drawText(QRect(-plotRect.height() / 2, -14, plotRect.height(), 18),
                             Qt::AlignCenter | Qt::AlignVCenter,
                             m_yAxisTitle);
            painter.restore();
        }
    }

private:
    QVector<AxisPerformanceSeries> m_series;
    QString m_xAxisTitle = QStringLiteral("Sample Index");
    QString m_xAxisLeftLabel = QStringLiteral("0");
    QString m_xAxisCenterLabel = QStringLiteral("0");
    QString m_xAxisRightLabel = QStringLiteral("0");
    QString m_yAxisTitle;
};

struct ChartAxisInfo
{
    QString xTitle = QStringLiteral("Sample Index");
    QString xLeft = QStringLiteral("0");
    QString xCenter = QStringLiteral("0");
    QString xRight = QStringLiteral("0");
    QString yTitle;
};

ChartAxisInfo buildChartAxisInfo(const AxisPerformanceImportedTestResult& test)
{
    ChartAxisInfo info;

    const auto formatAxisValue = [](double value) {
        if (!std::isfinite(value)) {
            return QStringLiteral("0");
        }
        return QString::number(value, 'f', value == std::floor(value) ? 0 : 3);
    };

    const auto buildSeriesTitle = [](const AxisPerformanceSeries& series) {
        const QString baseName = series.name.isEmpty() ? series.key : series.name;
        if (series.unit.isEmpty()) {
            return baseName;
        }
        return QStringLiteral("%1 (%2)").arg(baseName, series.unit);
    };

    for (const AxisPerformanceSeries& series : test.series) {
        if (series.renderStyle == QStringLiteral("marker")) {
            continue;
        }
        info.yTitle = buildSeriesTitle(series);
        break;
    }
    if (info.yTitle.isEmpty() && !test.series.isEmpty()) {
        info.yTitle = buildSeriesTitle(test.series.first());
    }

    if (test.key == QStringLiteral("speed_uniformity")) {
        const QString speedsText = test.meta.value(QStringLiteral("commandSpeeds"));
        const QStringList speedLabels = speedsText.split(',', Qt::SkipEmptyParts);
        info.xTitle = QStringLiteral("Command Speed (mm/s)");
        if (!speedLabels.isEmpty()) {
            info.xLeft = speedLabels.first().trimmed();
            info.xCenter = speedLabels.at(speedLabels.size() / 2).trimmed();
            info.xRight = speedLabels.last().trimmed();
        }
        return info;
    }

    if (test.key == QStringLiteral("static_jitter") || test.key == QStringLiteral("power_off_drop_distance")) {
        const int pointCount = test.series.isEmpty() ? 0 : test.series.first().values.size();
        info.xTitle = QStringLiteral("Point Index");
        if (pointCount > 0) {
            info.xCenter = QString::number(pointCount / 2);
            info.xRight = QString::number(std::max(pointCount - 1, 0));
        }
        return info;
    }

    if (test.key == QStringLiteral("settling_time")) {
        const int moveCount = test.series.isEmpty() ? 0 : test.series.first().values.size();
        info.xTitle = QStringLiteral("Move Index");
        if (moveCount > 0) {
            info.xCenter = QString::number(moveCount / 2);
            info.xRight = QString::number(std::max(moveCount - 1, 0));
        }
        return info;
    }

    if (test.key == QStringLiteral("dynamic_capability")) {
        const int sampleCount = test.series.isEmpty() ? 0 : test.series.first().values.size();
        bool ok = false;
        const double sampleIntervalMs = test.meta.value(QStringLiteral("sampleIntervalMs")).toDouble(&ok);
        info.xTitle = QStringLiteral("Time (ms)");
        if (ok && sampleCount > 1) {
            info.xCenter = formatAxisValue(((sampleCount - 1) / 2.0) * sampleIntervalMs);
            info.xRight = formatAxisValue((sampleCount - 1) * sampleIntervalMs);
        }
        return info;
    }

    if (test.key == QStringLiteral("limit_window")) {
        info.xTitle = QStringLiteral("Metric Index");
        const int metricCount = test.series.size();
        if (metricCount > 0) {
            info.xCenter = QString::number(metricCount / 2);
            info.xRight = QString::number(std::max(metricCount - 1, 0));
        }
        if (info.yTitle.isEmpty()) {
            info.yTitle = QStringLiteral("Position (mm)");
        }
        return info;
    }

    if (!test.series.isEmpty()) {
        const int sampleCount = test.series.first().values.size();
        bool ok = false;
        const double sampleIntervalMs = test.meta.value(QStringLiteral("sampleIntervalMs")).toDouble(&ok);
        if (ok && sampleCount > 1) {
            info.xTitle = QStringLiteral("Time (ms)");
            info.xCenter = formatAxisValue(((sampleCount - 1) / 2.0) * sampleIntervalMs);
            info.xRight = formatAxisValue((sampleCount - 1) * sampleIntervalMs);
        } else if (sampleCount > 0) {
            info.xCenter = QString::number(sampleCount / 2);
            info.xRight = QString::number(std::max(sampleCount - 1, 0));
        }
    }

    return info;
}

enum TestColumn
{
    TestNameColumn = 0,
    TestDescriptionColumn,
    TestColumnCount
};

enum ExecutionColumn
{
    ExecutionTestNameColumn = 0,
    ExecutionDescriptionColumn,
    ExecutionActionColumn,
    ExecutionColumnCount
};

enum ParameterColumn
{
    ParameterNameColumn = 0,
    ParameterValueColumn,
    ParameterUnitColumn,
    ParameterColumnCount
};

QString trText(const QString& languageCode, const QString& english, const QString& chinese)
{
    return languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0 ? english : chinese;
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

void mergeResultDocuments(AxisPerformanceImportedDocument* target, const AxisPerformanceImportedDocument& source)
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

QString buildInlineSeriesChartSvg(const QVector<AxisPerformanceSeries>& series)
{
    if (series.isEmpty()) {
        return QString();
    }

    double minValue = 0.0;
    double maxValue = 0.0;
    int maxCount = 0;
    bool hasValue = false;
    for (const AxisPerformanceSeries& item : series) {
        maxCount = std::max(maxCount, static_cast<int>(item.values.size()));
        for (double value : item.values) {
            if (!std::isfinite(value)) {
                continue;
            }
            if (!hasValue) {
                minValue = maxValue = value;
                hasValue = true;
            } else {
                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);
            }
        }
    }

    if (!hasValue || maxCount <= 0) {
        return QString();
    }
    if (std::abs(maxValue - minValue) < 1e-9) {
        maxValue += 1.0;
        minValue -= 1.0;
    }

    const QVector<QString> palette = {
        QStringLiteral("#0f766e"),
        QStringLiteral("#2563eb"),
        QStringLiteral("#d97706"),
        QStringLiteral("#dc2626"),
        QStringLiteral("#7c3aed")
    };

    const double width = 760.0;
    const double left = 50.0;
    const double top = 20.0;
    const double plotWidth = 680.0;
    const double legendRowHeight = 18.0;
    const double legendTopGap = 26.0;
    const double legendLeft = left;
    const double legendAvailableWidth = plotWidth;

    struct LegendEntry
    {
        QString text;
        QString color;
        double width = 0.0;
    };

    QVector<LegendEntry> legends;
    legends.reserve(series.size());
    for (int index = 0; index < series.size(); ++index) {
        const AxisPerformanceSeries& item = series.at(index);
        QString color = palette.at(index % palette.size());
        if (item.renderStyle == QStringLiteral("marker")) {
            color = QStringLiteral("#dc2626");
        } else if (item.renderStyle == QStringLiteral("range_highlight")) {
            color = QStringLiteral("#d97706");
        }
        LegendEntry entry;
        entry.text = (item.name.isEmpty() ? item.key : item.name).toHtmlEscaped();
        entry.color = color;
        entry.width = 24.0 + (entry.text.size() * 7.2);
        legends.push_back(entry);
    }

    int legendRows = legends.isEmpty() ? 0 : 1;
    double legendCursor = 0.0;
    for (const LegendEntry& entry : legends) {
        if (legendCursor > 0.0 && (legendCursor + entry.width) > legendAvailableWidth) {
            legendRows += 1;
            legendCursor = 0.0;
        }
        legendCursor += entry.width + 14.0;
    }

    const double legendHeight = legendRows > 0 ? (legendRows * legendRowHeight) + 6.0 : 0.0;
    const double plotHeight = 170.0;
    const double height = 240.0 + legendHeight;
    const double svgPlotBottom = top + plotHeight;

    QString svg;
    QTextStream stream(&svg);
    stream.setEncoding(QStringConverter::Utf8);
    stream << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width << "\" height=\"" << height << "\" viewBox=\"0 0 "
           << width << ' ' << height << "\" style=\"max-width:100%;height:auto;display:block\">";
    stream << "<rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height << "\" fill=\"#ffffff\" stroke=\"#d7dee5\" rx=\"10\" ry=\"10\"/>";
    stream << "<defs><clipPath id=\"plotClip\"><rect x=\"" << left << "\" y=\"" << top << "\" width=\"" << plotWidth << "\" height=\"" << plotHeight << "\"/></clipPath></defs>";
    for (int row = 0; row < 5; ++row) {
        const double y = top + (plotHeight * row / 4.0);
        stream << "<line x1=\"" << left << "\" y1=\"" << y << "\" x2=\"" << (left + plotWidth) << "\" y2=\"" << y
               << "\" stroke=\"#eef2f6\" stroke-width=\"1\"/>";
    }
    stream << "<line x1=\"" << left << "\" y1=\"" << svgPlotBottom << "\" x2=\"" << (left + plotWidth) << "\" y2=\"" << svgPlotBottom
           << "\" stroke=\"#a8b4bf\" stroke-width=\"1.2\"/>";
    stream << "<line x1=\"" << left << "\" y1=\"" << top << "\" x2=\"" << left << "\" y2=\"" << svgPlotBottom
           << "\" stroke=\"#a8b4bf\" stroke-width=\"1.2\"/>";
    stream << "<text x=\"10\" y=\"" << (top + 6) << "\" fill=\"#526372\" font-size=\"11\">" << QString::number(maxValue, 'f', 3).toHtmlEscaped() << "</text>";
    stream << "<text x=\"10\" y=\"" << svgPlotBottom << "\" fill=\"#526372\" font-size=\"11\">" << QString::number(minValue, 'f', 3).toHtmlEscaped() << "</text>";

    int colorIndex = 0;
    stream << "<g clip-path=\"url(#plotClip)\">";
    for (const AxisPerformanceSeries& item : series) {
        QString color = palette.at(colorIndex % palette.size());
        if (item.renderStyle == QStringLiteral("marker")) {
            color = QStringLiteral("#dc2626");
        } else if (item.renderStyle == QStringLiteral("range_highlight")) {
            color = QStringLiteral("#d97706");
        }

        QString polylinePoints;
        struct PointData { double x; double y; };
        QVector<PointData> points;
        points.reserve(item.values.size());
        for (int index = 0; index < item.values.size(); ++index) {
            const double value = item.values.at(index);
            if (!std::isfinite(value)) {
                continue;
            }
            const double xRatio = maxCount <= 1 ? 0.0 : static_cast<double>(index) / static_cast<double>(maxCount - 1);
            const double yRatio = (value - minValue) / (maxValue - minValue);
            const double x = left + xRatio * plotWidth;
            const double y = top + plotHeight - yRatio * plotHeight;
            points.push_back({x, y});
            if (!polylinePoints.isEmpty()) {
                polylinePoints += ' ';
            }
            polylinePoints += QStringLiteral("%1,%2").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2);
        }

        if (!polylinePoints.isEmpty() && item.renderStyle != QStringLiteral("marker")) {
            stream << "<polyline fill=\"none\" stroke=\"" << color << "\" stroke-width=\"" << (item.renderStyle == QStringLiteral("range_highlight") ? 4 : 2)
                   << "\" points=\"" << polylinePoints << "\"/>";
        }
        for (const PointData& point : points) {
            const double radius = item.renderStyle == QStringLiteral("marker") ? 4.5 : 2.5;
            stream << "<circle cx=\"" << point.x << "\" cy=\"" << point.y << "\" r=\"" << radius
                   << "\" fill=\"" << color << "\" stroke=\"#ffffff\" stroke-width=\"1\"/>";
        }

        colorIndex += 1;
    }
    stream << "</g>";

    double legendX = legendLeft;
    double legendY = svgPlotBottom + legendTopGap;
    for (const LegendEntry& entry : legends) {
        if (legendX > legendLeft && (legendX + entry.width) > (legendLeft + legendAvailableWidth)) {
            legendX = legendLeft;
            legendY += legendRowHeight;
        }
        stream << "<rect x=\"" << legendX << "\" y=\"" << (legendY - 10.0) << "\" width=\"12\" height=\"12\" fill=\"" << entry.color << "\"/>";
        stream << "<text x=\"" << (legendX + 16.0) << "\" y=\"" << legendY << "\" fill=\"#31424f\" font-size=\"12\">" << entry.text << "</text>";
        legendX += entry.width + 14.0;
    }

    stream << "</svg>";
    return svg;
}

QString buildInlineSeriesChartDataUrl(const QVector<AxisPerformanceSeries>& series, const ChartAxisInfo& axisInfo)
{
    if (series.isEmpty()) {
        return QString();
    }

    AxisPerformanceChartWidget chartWidget;
    chartWidget.resize(760, 320);
    chartWidget.setSeries(series);
    chartWidget.setXAxisInfo(axisInfo.xTitle, axisInfo.xLeft, axisInfo.xCenter, axisInfo.xRight);
    chartWidget.setYAxisInfo(axisInfo.yTitle);

    QImage image(chartWidget.size() * chartWidget.devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(chartWidget.devicePixelRatioF());
    image.fill(Qt::white);

    QPainter painter(&image);
    chartWidget.render(&painter);
    painter.end();

    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return QStringLiteral("data:image/png;base64,%1").arg(QString::fromLatin1(pngBytes.toBase64()));
}

QTableWidgetItem* makeItem(const QString& text)
{
    return new QTableWidgetItem(text);
}

double meanOf(const QVector<double>& values)
{
    double sum = 0.0;
    int count = 0;
    for (double value : values) {
        if (!std::isfinite(value)) {
            continue;
        }
        sum += value;
        ++count;
    }
    return count == 0 ? 0.0 : sum / static_cast<double>(count);
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

double stddevOf(const QVector<double>& values, double mean)
{
    double sum = 0.0;
    int count = 0;
    for (double value : values) {
        if (!std::isfinite(value)) {
            continue;
        }
        const double delta = value - mean;
        sum += delta * delta;
        ++count;
    }
    return count <= 1 ? 0.0 : std::sqrt(sum / static_cast<double>(count - 1));
}

QPair<int, int> findConstantVelocitySegment(const QVector<double>& values)
{
    QVector<double> absoluteVelocities;
    absoluteVelocities.reserve(values.size());
    double maxVelocity = 0.0;
    for (double value : values) {
        const double absValue = std::abs(value);
        absoluteVelocities.push_back(absValue);
        if (std::isfinite(absValue)) {
            maxVelocity = std::max(maxVelocity, absValue);
        }
    }

    if (absoluteVelocities.isEmpty()) {
        return {-1, -1};
    }

    const double threshold = maxVelocity * 0.9;
    int bestStart = -1;
    int bestEnd = -1;
    int currentStart = -1;
    for (int index = 0; index < absoluteVelocities.size(); ++index) {
        const double value = absoluteVelocities.at(index);
        const bool inSegment = maxVelocity <= 0.0 || (std::isfinite(value) && value >= threshold);
        if (inSegment) {
            if (currentStart < 0) {
                currentStart = index;
            }
            if (bestStart < 0 || (index - currentStart) > (bestEnd - bestStart)) {
                bestStart = currentStart;
                bestEnd = index;
            }
        } else {
            currentStart = -1;
        }
    }

    if (bestStart < 0) {
        bestStart = 0;
        bestEnd = absoluteVelocities.size() - 1;
    }
    return {bestStart, bestEnd};
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

QString axisSummaryText(const AxisPerformanceAxis& axis)
{
    const QString axisName = axis.axisName.trimmed().isEmpty() ? QStringLiteral("Axis") : axis.axisName.trimmed();
    return QStringLiteral("%1 (#%2)").arg(axisName).arg(axis.axisNumber);
}

QString defaultParameterSetName(int index)
{
    return QString::fromUtf8(u8"参数组 %1").arg(index + 1);
}

QString localizedTestName(const QString& languageCode, const QString& testKey, const QString& fallback)
{
    if (languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0) {
        return fallback;
    }

    if (testKey == QStringLiteral("limit_window")) return QString::fromUtf8(u8"软限位设置");
    if (testKey == QStringLiteral("static_jitter")) return QString::fromUtf8(u8"静态抖动");
    if (testKey == QStringLiteral("settling_time")) return QString::fromUtf8(u8"稳定时间");
    if (testKey == QStringLiteral("speed_uniformity")) return QString::fromUtf8(u8"速度均匀性");
    if (testKey == QStringLiteral("dynamic_capability")) return QString::fromUtf8(u8"动态能力");
    if (testKey == QStringLiteral("power_off_drop_distance")) return QString::fromUtf8(u8"断电下落距离");
    return fallback;
}

QString localizedParameterName(const QString& languageCode, const QString& parameterKey, const QString& fallback)
{
    if (languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) == 0) {
        return fallback;
    }

    if (parameterKey == QStringLiteral("electrical_negative")) return QString::fromUtf8(u8"电气负限位");
    if (parameterKey == QStringLiteral("electrical_positive")) return QString::fromUtf8(u8"电气正限位");
    if (parameterKey == QStringLiteral("software_negative")) return QString::fromUtf8(u8"软件负限位");
    if (parameterKey == QStringLiteral("software_positive")) return QString::fromUtf8(u8"软件正限位");
    if (parameterKey == QStringLiteral("stroke")) return QString::fromUtf8(u8"行程");
    if (parameterKey == QStringLiteral("servo_frequency")) return QString::fromUtf8(u8"伺服频率");
    if (parameterKey == QStringLiteral("max_jitter")) return QString::fromUtf8(u8"最大抖动");
    if (parameterKey == QStringLiteral("error_window")) return QString::fromUtf8(u8"误差窗口");
    if (parameterKey == QStringLiteral("step_distance")) return QString::fromUtf8(u8"步进距离");
    if (parameterKey == QStringLiteral("max_time")) return QString::fromUtf8(u8"最大时间");
    if (parameterKey == QStringLiteral("speed_min")) return QString::fromUtf8(u8"最小速度");
    if (parameterKey == QStringLiteral("speed_max")) return QString::fromUtf8(u8"最大速度");
    if (parameterKey == QStringLiteral("max_variation")) return QString::fromUtf8(u8"最大波动");
    if (parameterKey == QStringLiteral("max_velocity")) return QString::fromUtf8(u8"最高速度");
    if (parameterKey == QStringLiteral("max_acceleration")) return QString::fromUtf8(u8"最大加速度");
    if (parameterKey == QStringLiteral("max_jerk")) return QString::fromUtf8(u8"最大加加速度");
    if (parameterKey == QStringLiteral("max_drop")) return QString::fromUtf8(u8"最大下落距离");
    return fallback;
}

QString localizedParameterSetName(const QString& languageCode, const QString& name, int index)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return defaultParameterSetName(index);
    }

    if (languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) != 0) {
        static const QRegularExpression setPattern(QStringLiteral("^Set\\s+(\\d+)$"),
                                                   QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = setPattern.match(trimmed);
        if (match.hasMatch()) {
            return QString::fromUtf8(u8"参数组 %1").arg(match.captured(1));
        }
    }

    return trimmed;
}
}

AxisPerformancePage::AxisPerformancePage(ControllerService& controllerService, QWidget* parent)
    : LoggedPageWidget(FileLogger::Category::AxisPerformance, parent)
    , m_controllerService(controllerService)
    , m_executionService(controllerService)
    , m_configWatcher(new QFileSystemWatcher(this))
{
    buildUi();
    reloadConfig();
}

void AxisPerformancePage::setLanguage(const QString& languageCode)
{
    if (m_languageCode == languageCode) {
        return;
    }

    m_languageCode = languageCode;
    retranslateUi();
}

void AxisPerformancePage::reloadConfig()
{
    QString errorMessage;
    const QVector<AxisPerformanceProfile> profiles = m_service.loadProfiles(configFilePath(), &errorMessage);
    if (profiles.isEmpty() && !errorMessage.isEmpty()) {
        showStatusMessage(errorMessage, true);
        return;
    }

    m_profiles = profiles;
    if (m_editorDialog && m_editorDialog->isVisible()) {
        loadEditorFromProfiles();
    }
    refreshExecutionAxisSelector();
    refreshExecutionTable();
    populateResultSelectors();
    refreshResultView();
    updateWatcher();
}

void AxisPerformancePage::openConfigEditorDialog()
{
    if (!m_editorDialog) {
        buildConfigEditorDialog();
        retranslateUi();
    }

    loadEditorFromProfiles();
    m_editorDialog->show();
    m_editorDialog->raise();
    m_editorDialog->activateWindow();
}

void AxisPerformancePage::browseConfigFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        trText(m_languageCode, QStringLiteral("Select Axis Performance Config"), QStringLiteral("Select Axis Performance Config")),
        QFileInfo(configFilePath()).absolutePath(),
        QStringLiteral("JSON (*.json)"));
    if (path.isEmpty()) {
        return;
    }

    m_editorPathLabel->setProperty("configPath", path);
    m_editorPathLabel->setText(trText(m_languageCode,
                                      QStringLiteral("Config: %1").arg(path),
                                      QString::fromUtf8(u8"\u914d\u7f6e\u6587\u4ef6\uff1a%1").arg(path)));
    reloadConfig();
}

void AxisPerformancePage::saveConfig()
{
    QString errorMessage;
    if (!validateProfiles(m_editorProfiles, &errorMessage)) {
        showStatusMessage(errorMessage, true);
        return;
    }

    if (!m_service.saveProfiles(configFilePath(), m_editorProfiles, &errorMessage)) {
        showStatusMessage(errorMessage, true);
        return;
    }

    m_profiles = m_editorProfiles;
    refreshEditor();
    updateWatcher();
    showStatusMessage(trText(m_languageCode,
                             QStringLiteral("Axis performance config saved successfully"),
                             QString::fromUtf8(u8"\u8F74\u6027\u80FD\u914D\u7F6E\u4FDD\u5B58\u6210\u529F")),
                      false);
}

void AxisPerformancePage::handleConfigFileChanged(const QString& path)
{
    Q_UNUSED(path);
    reloadConfig();
}

void AxisPerformancePage::handleEditorProfileChanged(int)
{
    if (m_isRefreshingEditor) {
        return;
    }
    refreshEditorProfileSection();
    refreshAxisList();
}

void AxisPerformancePage::handleProfileNameEdited(const QString& text)
{
    if (m_isRefreshingEditor) {
        return;
    }

    AxisPerformanceProfile* profile = currentEditorProfile();
    if (!profile) {
        return;
    }

    profile->profileName = text.trimmed();
    const int index = currentEditorProfileIndex();
    if (index >= 0 && m_editorProfileCombo) {
        QSignalBlocker blocker(m_editorProfileCombo);
        m_editorProfileCombo->setItemText(index, profile->profileName.isEmpty() ? textForKey(QStringLiteral("unnamed_profile")) : profile->profileName);
        m_editorProfileCombo->setCurrentIndex(index);
    }
}

void AxisPerformancePage::handleProfileDescriptionEdited(const QString& text)
{
    if (m_isRefreshingEditor) {
        return;
    }

    AxisPerformanceProfile* profile = currentEditorProfile();
    if (profile) {
        profile->description = text.trimmed();
    }
}

void AxisPerformancePage::addProfile()
{
    AxisPerformanceProfile profile;
    profile.profileName = trText(m_languageCode, QStringLiteral("New Profile"), QString::fromUtf8(u8"新建配置"));
    profile.description = trText(m_languageCode, QStringLiteral("Axis performance profile"), QString::fromUtf8(u8"轴性能测试配置"));
    m_editorProfiles.push_back(profile);
    refreshEditor();
    if (m_editorProfileCombo) {
        m_editorProfileCombo->setCurrentIndex(m_editorProfileCombo->count() - 1);
    }
}

void AxisPerformancePage::removeProfile()
{
    const int index = currentEditorProfileIndex();
    if (index < 0 || index >= m_editorProfiles.size()) {
        return;
    }

    m_editorProfiles.removeAt(index);
    refreshEditor();
}

void AxisPerformancePage::handleAxisSelectionChanged()
{
    if (m_isRefreshingEditor) {
        return;
    }
    refreshAxisDetail();
}

void AxisPerformancePage::handleAxisNameEdited(const QString& text)
{
    if (m_isRefreshingEditor) {
        return;
    }

    AxisPerformanceAxis* axis = currentEditorAxis();
    if (!axis) {
        return;
    }

    axis->axisName = text.trimmed();
    const int row = currentEditorAxisIndex();
    if (row >= 0 && m_axisListWidget) {
        QListWidgetItem* item = m_axisListWidget->item(row);
        if (item) {
            item->setText(axisSummaryText(*axis));
        }
    }
}

void AxisPerformancePage::handleAxisNumberEdited(int value)
{
    if (m_isRefreshingEditor) {
        return;
    }

    AxisPerformanceAxis* axis = currentEditorAxis();
    if (!axis) {
        return;
    }

    axis->axisNumber = value;
    const int row = currentEditorAxisIndex();
    if (row >= 0 && m_axisListWidget) {
        QListWidgetItem* item = m_axisListWidget->item(row);
        if (item) {
            item->setText(axisSummaryText(*axis));
        }
    }
}

void AxisPerformancePage::addAxis()
{
    AxisPerformanceProfile* profile = currentEditorProfile();
    if (!profile) {
        return;
    }

    AxisPerformanceAxis axis;
    axis.axisName = QStringLiteral("Axis");
    axis.axisNumber = profile->axes.size();
    axis.unit = QStringLiteral("mm");
    profile->axes.push_back(axis);

    refreshAxisList();
    if (m_axisListWidget) {
        m_axisListWidget->setCurrentRow(m_axisListWidget->count() - 1);
    }
}

void AxisPerformancePage::removeAxis()
{
    AxisPerformanceProfile* profile = currentEditorProfile();
    const int axisIndex = currentEditorAxisIndex();
    if (!profile || axisIndex < 0 || axisIndex >= profile->axes.size()) {
        return;
    }

    profile->axes.removeAt(axisIndex);
    refreshAxisList();
}

void AxisPerformancePage::handleTestSelectionChanged()
{
    if (m_isRefreshingEditor) {
        return;
    }
    if (m_removeTestButton) {
        m_removeTestButton->setEnabled(currentEditorTestItem() != nullptr);
    }
    refreshParameterSetSection();
    refreshParameterTable();
}

void AxisPerformancePage::addTestItem()
{
    AxisPerformanceAxis* axis = currentEditorAxis();
    if (!axis || !m_testTemplateCombo) {
        return;
    }

    const QString testKey = m_testTemplateCombo->currentData().toString();
    if (testKey.isEmpty() || axisHasTestKey(*axis, testKey)) {
        return;
    }

    AxisPerformanceTestItem testItem;
    applyTestTemplate(&testItem, testKey);
    if (testItem.key.isEmpty()) {
        return;
    }

    axis->testItems.push_back(testItem);
    refreshTestTable();
    if (m_testTable) {
        m_testTable->setCurrentCell(m_testTable->rowCount() - 1, 0);
    }
}

void AxisPerformancePage::removeTestItem()
{
    AxisPerformanceAxis* axis = currentEditorAxis();
    const int row = currentEditorTestIndex();
    if (!axis || row < 0 || row >= axis->testItems.size()) {
        return;
    }

    axis->testItems.removeAt(row);
    refreshTestTable();
}

void AxisPerformancePage::handleParameterSetChanged(int)
{
    if (m_isRefreshingEditor) {
        return;
    }
    refreshParameterTable();
}

void AxisPerformancePage::addParameterSet()
{
    AxisPerformanceTestItem* testItem = currentEditorTestItem();
    if (!testItem || !supportsParameterSets(*testItem)) {
        return;
    }

    QVector<AxisPerformanceParameter> templateParameters;
    if (!testItem->parameterSets.isEmpty()) {
        templateParameters = testItem->parameterSets.constLast().parameters;
        for (AxisPerformanceParameter& parameter : templateParameters) {
            parameter.value.clear();
        }
    } else if (!testItem->parameters.isEmpty()) {
        templateParameters = testItem->parameters;
        for (AxisPerformanceParameter& parameter : templateParameters) {
            parameter.value.clear();
        }
    }

    AxisPerformanceParameterSet parameterSet;
    parameterSet.name = defaultParameterSetName(testItem->parameterSets.size());
    parameterSet.parameters = templateParameters;
    testItem->parameterSets.push_back(parameterSet);

    refreshParameterSetSection();
    if (m_parameterSetCombo) {
        m_parameterSetCombo->setCurrentIndex(m_parameterSetCombo->count() - 1);
    }
}

void AxisPerformancePage::removeParameterSet()
{
    AxisPerformanceTestItem* testItem = currentEditorTestItem();
    const int index = currentEditorParameterSetIndex();
    if (!testItem || !supportsParameterSets(*testItem) || index < 0 || index >= testItem->parameterSets.size()) {
        return;
    }

    testItem->parameterSets.removeAt(index);
    refreshParameterSetSection();
    refreshParameterTable();
}

void AxisPerformancePage::handleParameterItemChanged(QTableWidgetItem* item)
{
    if (m_isRefreshingEditor || !item) {
        return;
    }

    QVector<AxisPerformanceParameter>* parameters = currentEditorParameters();
    if (!parameters) {
        return;
    }

    const int row = item->row();
    if (row < 0 || row >= parameters->size()) {
        return;
    }

    if (item->column() == ParameterValueColumn) {
        (*parameters)[row].value = item->text().trimmed();
    }
}

void AxisPerformancePage::addParameter()
{
}

void AxisPerformancePage::removeParameter()
{
}

void AxisPerformancePage::importResultFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        trText(m_languageCode, QStringLiteral("Import Axis Performance Results"), QStringLiteral("Import Axis Performance Results")),
        QString(),
        QStringLiteral("JSON (*.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!loadResultDocument(filePath, &errorMessage)) {
        showStatusMessage(errorMessage.isEmpty()
                              ? trText(m_languageCode, QStringLiteral("Failed to load result file"), QString::fromUtf8(u8"\u52A0\u8F7D\u7ED3\u679C\u6587\u4EF6\u5931\u8D25"))
                              : errorMessage,
                          true);
        return;
    }

    populateResultSelectors();
    refreshResultView();
    showStatusMessage(trText(m_languageCode, QStringLiteral("Result file loaded"), QString::fromUtf8(u8"结果文件已加载")), false);
}

void AxisPerformancePage::exportReport()
{
    if (m_resultDocument.axes.isEmpty()) {
        showStatusMessage(trText(m_languageCode, QStringLiteral("No imported results to export"), QString::fromUtf8(u8"没有可导出的导入结果")), true);
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this,
        trText(m_languageCode, QStringLiteral("Export Axis Performance Report"), QString::fromUtf8(u8"导出轴性能报告")),
        QStringLiteral("axis_performance_report.html"),
        QStringLiteral("HTML (*.html)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        showStatusMessage(file.errorString(), true);
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << buildReportHtml();
    file.close();

    showStatusMessage(trText(m_languageCode, QStringLiteral("Report exported"), QString::fromUtf8(u8"报告已导出")), false);
}

void AxisPerformancePage::handleExecutionAxisChanged(int)
{
    refreshExecutionTable();
}

void AxisPerformancePage::handleExecutionSelectionChanged()
{
    const bool hasSelection = m_executionTestCombo && m_executionTestCombo->currentIndex() >= 0;
    if (m_runSelectedTestButton) {
        m_runSelectedTestButton->setEnabled(hasSelection);
    }
    if (m_openSelectedScriptButton) {
        m_openSelectedScriptButton->setEnabled(hasSelection);
    }
}

void AxisPerformancePage::handleResultAxisChanged(int)
{
    if (m_resultAxisCombo) {
        populateResultTestSelector(m_resultAxisCombo->currentData().toInt());
    }
    refreshResultView();
}

void AxisPerformancePage::handleResultTestChanged(int)
{
    refreshResultView();
}

void AxisPerformancePage::openTestScript()
{
    if (!m_executionAxisCombo || !m_executionTestCombo || m_profiles.isEmpty()) {
        return;
    }
    const int axisIndex = m_executionAxisCombo->currentData().toInt();
    const int testIndex = m_executionTestCombo->currentData().toInt();
    const AxisPerformanceProfile& profile = m_profiles.first();
    if (axisIndex < 0 || axisIndex >= profile.axes.size()) {
        return;
    }
    const AxisPerformanceAxis& axis = profile.axes.at(axisIndex);
    if (testIndex < 0 || testIndex >= axis.testItems.size()) {
        return;
    }
    const QString testKey = axis.testItems.at(testIndex).key;
    const QString scriptPath = m_workflowService.scriptPathForTestKey(testKey);
    const QString labelName = m_workflowService.labelForTestKey(testKey);
    if (scriptPath.isEmpty() || !QFileInfo::exists(scriptPath)) {
        showStatusMessage(trText(m_languageCode, QStringLiteral("Test script not found"), QString::fromUtf8(u8"未找到测试脚本")), true);
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(scriptPath));
    logMessage(trText(m_languageCode,
                      QStringLiteral("Opened suite buffer: %1, label=%2").arg(scriptPath, labelName),
                      QString::fromUtf8(u8"已打开 suite buffer：%1，label=%2").arg(scriptPath, labelName)));
}

void AxisPerformancePage::runTestItem()
{
    if (!m_executionAxisCombo || !m_executionTestCombo || m_profiles.isEmpty()) {
        return;
    }
    if (m_runSelectedTestButton && m_runSelectedTestButton->property("testRunning").toBool()) {
        return;
    }
    const int axisIndex = m_executionAxisCombo->currentData().toInt();
    const int testIndex = m_executionTestCombo->currentData().toInt();
    if (axisIndex < 0 || testIndex < 0) {
        return;
    }
    const AxisPerformanceProfile& profile = m_profiles.first();
    if (axisIndex >= profile.axes.size()) {
        return;
    }
    const AxisPerformanceAxis& axis = profile.axes.at(axisIndex);
    if (testIndex >= axis.testItems.size()) {
        return;
    }
    const AxisPerformanceTestItem& testItem = axis.testItems.at(testIndex);
    const QString labelName = m_workflowService.labelForTestKey(testItem.key);
    if (labelName.isEmpty()) {
        showStatusMessage(trText(m_languageCode, QStringLiteral("Unknown suite label"), QString::fromUtf8(u8"未知的 suite label")), true);
        return;
    }
    if (!m_controllerService.isConnected()) {
        showStatusMessage(trText(m_languageCode, QStringLiteral("Controller is not connected"), QString::fromUtf8(u8"控制器未连接")), true);
        return;
    }
    const QString testDisplayName = displayNameForTestKey(testItem.key);
    showStatusMessage(
        trText(m_languageCode,
               QStringLiteral("Running %1 on %2 (#%3)...").arg(testDisplayName, axis.axisName).arg(axis.axisNumber),
               QString::fromUtf8(u8"正在对 %2 (#%3) 运行 %1...").arg(testDisplayName, axis.axisName).arg(axis.axisNumber)),
        false);

    if (m_runSelectedTestButton) {
        m_runSelectedTestButton->setProperty("testRunning", true);
        m_runSelectedTestButton->setEnabled(false);
    }
    if (m_openSelectedScriptButton) {
        m_openSelectedScriptButton->setEnabled(false);
    }

    auto* watcher = new QFutureWatcher<AxisPerformanceRunOutcome>(this);
    QPointer<AxisPerformancePage> self(this);
    connect(watcher, &QFutureWatcher<AxisPerformanceRunOutcome>::finished, this, [self, watcher]() {
        AxisPerformanceRunOutcome outcome = watcher->result();
        watcher->deleteLater();
        if (!self) {
            return;
        }

        if (self->m_runSelectedTestButton) {
            self->m_runSelectedTestButton->setProperty("testRunning", false);
        }
        self->handleExecutionSelectionChanged();

        if (!outcome.ok) {
            self->showStatusMessage(
                outcome.errorMessage.isEmpty()
                    ? trText(self->m_languageCode, QStringLiteral("Failed to run test"), QString::fromUtf8(u8"运行测试失败"))
                    : outcome.errorMessage,
                true);
            return;
        }

        self->m_workflowService.recomputeDocumentMetrics(&outcome.liveDocument, self->m_profiles, self->m_languageCode);
        self->m_workflowService.mergeResultDocuments(&self->m_resultDocument, outcome.liveDocument);
        self->populateResultSelectors();
        self->selectResultViewForTest(outcome.axisNumber, outcome.testKey);
        if ((self->m_resultAxisCombo && self->m_resultAxisCombo->count() == 0)
            || (self->m_resultTestCombo && self->m_resultTestCombo->count() == 0)) {
            self->m_resultDocument = outcome.liveDocument;
            self->populateResultSelectors();
            self->selectResultViewForTest(outcome.axisNumber, outcome.testKey);
        }
        self->refreshResultView();
        if (self->m_resultAxisCombo && self->m_resultAxisCombo->count() == 0 && !self->m_resultDocument.axes.isEmpty()) {
            self->populateResultSelectors();
        }
        if (self->m_resultTestCombo && self->m_resultTestCombo->count() == 0 && !self->m_resultDocument.axes.isEmpty()) {
            const AxisPerformanceImportedAxisResult& fallbackAxis = self->m_resultDocument.axes.first();
            self->populateResultTestSelector(fallbackAxis.axisNumber, outcome.testKey);
        }
        if (self->m_reportPreview && !self->m_reportPreview->isVisible()) {
            self->refreshResultView();
        }
        self->setReportPanelVisible(true);
        self->showStatusMessage(
            trText(self->m_languageCode,
                   QStringLiteral("Test completed: %1").arg(outcome.testDisplayName),
                   QString::fromUtf8(u8"测试完成：%1").arg(outcome.testDisplayName)),
            false);
    });

    watcher->setFuture(QtConcurrent::run([this, axis, testItem, testDisplayName]() {
        AxisPerformanceRunOutcome outcome;
        outcome.axisNumber = axis.axisNumber;
        outcome.testKey = testItem.key;
        outcome.testDisplayName = testDisplayName;
        outcome.ok = m_executionService.runControllerBackedTest(axis, testItem, &outcome.liveDocument, &outcome.errorMessage);
        return outcome;
    }));
}

bool AxisPerformancePage::loadResultDocument(const QString& filePath, QString* errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    AxisPerformanceImportedDocument document = m_workflowService.parseResultDocument(data, errorMessage);
    if (document.axes.isEmpty()) {
        if (errorMessage && errorMessage->isEmpty()) {
            *errorMessage = trText(m_languageCode, QStringLiteral("No axis results found"), QString::fromUtf8(u8"\u672A\u627E\u5230\u8F74\u6D4B\u8BD5\u7ED3\u679C"));
        }
        return false;
    }

    document.sourceFile = filePath;
    m_workflowService.recomputeDocumentMetrics(&document, m_profiles, m_languageCode);

    m_resultDocument = document;
    return true;
}

AxisPerformanceImportedDocument AxisPerformancePage::parseResultDocument(const QByteArray& data, QString* errorMessage) const
{
    return m_workflowService.parseResultDocument(data, errorMessage);
}

AxisPerformanceImportedTestResult AxisPerformancePage::parseImportedTest(const QVariantMap& testMap) const
{
    return m_workflowService.parseImportedTest(testMap);
}

void AxisPerformancePage::computeMetrics(AxisPerformanceImportedTestResult* testResult, const AxisPerformanceAxis* configuredAxis) const
{
    m_workflowService.computeMetrics(testResult, configuredAxis, m_languageCode);
}

const AxisPerformanceAxis* AxisPerformancePage::findConfiguredAxis(const AxisPerformanceImportedAxisResult& axisResult) const
{
    return m_workflowService.findConfiguredAxis(axisResult, m_profiles);
}

const AxisPerformanceTestItem* AxisPerformancePage::findConfiguredTest(const AxisPerformanceAxis& axis, const QString& testKey) const
{
    return m_workflowService.findConfiguredTest(axis, testKey);
}

double AxisPerformancePage::parameterValue(const AxisPerformanceTestItem* testItem, const QString& key, int parameterSetIndex, double fallback) const
{
    return m_workflowService.parameterValue(testItem, key, parameterSetIndex, fallback);
}

QString AxisPerformancePage::scriptPathForTestKey(const QString& testKey) const
{
    return m_workflowService.scriptPathForTestKey(testKey);
}

QString AxisPerformancePage::labelForTestKey(const QString& testKey) const
{
    return m_workflowService.labelForTestKey(testKey);
}

void AxisPerformancePage::refreshExecutionAxisSelector()
{
    if (!m_executionAxisCombo) {
        return;
    }

    const int previousAxisNumber = m_executionAxisCombo->currentData().toInt();
    QSignalBlocker blocker(m_executionAxisCombo);
    m_executionAxisCombo->clear();
    if (m_profiles.isEmpty()) {
        return;
    }

    const AxisPerformanceProfile& profile = m_profiles.first();
    for (int i = 0; i < profile.axes.size(); ++i) {
        const AxisPerformanceAxis& axis = profile.axes.at(i);
        m_executionAxisCombo->addItem(QStringLiteral("%1 (#%2)").arg(axis.axisName).arg(axis.axisNumber), i);
        if (axis.axisNumber == previousAxisNumber) {
            m_executionAxisCombo->setCurrentIndex(m_executionAxisCombo->count() - 1);
        }
    }
    if (m_executionAxisCombo->currentIndex() < 0 && m_executionAxisCombo->count() > 0) {
        m_executionAxisCombo->setCurrentIndex(0);
    }
}

void AxisPerformancePage::refreshExecutionTable()
{
    if (!m_executionTestCombo) {
        return;
    }

    const QString previousTest = m_executionTestCombo->currentData().toString();
    if (m_profiles.isEmpty() || !m_executionAxisCombo || m_executionAxisCombo->count() == 0) {
        QSignalBlocker blocker(m_executionTestCombo);
        m_executionTestCombo->clear();
        handleExecutionSelectionChanged();
        return;
    }

    const AxisPerformanceProfile& profile = m_profiles.first();
    const int axisIndex = m_executionAxisCombo->currentData().toInt();
    if (axisIndex < 0 || axisIndex >= profile.axes.size()) {
        QSignalBlocker blocker(m_executionTestCombo);
        m_executionTestCombo->clear();
        handleExecutionSelectionChanged();
        return;
    }

    const AxisPerformanceAxis& axis = profile.axes.at(axisIndex);
    QSignalBlocker blocker(m_executionTestCombo);
    m_executionTestCombo->clear();
    for (int testIndex = 0; testIndex < axis.testItems.size(); ++testIndex) {
        const AxisPerformanceTestItem& testItem = axis.testItems.at(testIndex);
        const QString label = testItem.description.trimmed().isEmpty()
                                  ? displayNameForTestKey(testItem.key)
                                  : QStringLiteral("%1 - %2").arg(displayNameForTestKey(testItem.key), testItem.description);
        m_executionTestCombo->addItem(label, testIndex);
    }
    int comboIndex = previousTest.isEmpty() ? 0 : m_executionTestCombo->findData(previousTest);
    if (comboIndex < 0) {
        comboIndex = 0;
    }
    if (m_executionTestCombo->count() > 0) {
        m_executionTestCombo->setCurrentIndex(comboIndex);
    }
    handleExecutionSelectionChanged();
}

void AxisPerformancePage::setReportPanelVisible(bool visible)
{
    if (m_analysisCard) {
        m_analysisCard->setVisible(visible);
    }
}

void AxisPerformancePage::populateResultSelectors()
{
    if (!m_resultAxisCombo || !m_resultTestCombo) {
        return;
    }

    const int currentAxisNumber = m_resultAxisCombo->currentData().toInt();
    const QString currentTestKey = m_resultTestCombo->currentData().toString();
    {
        QSignalBlocker blocker(m_resultAxisCombo);
        m_resultAxisCombo->clear();
        int axisIndexToSelect = 0;
        for (int i = 0; i < m_resultDocument.axes.size(); ++i) {
            const AxisPerformanceImportedAxisResult& axis = m_resultDocument.axes.at(i);
            const QString label = axis.axisName.isEmpty()
                                      ? QStringLiteral("Axis #%1").arg(axis.axisNumber)
                                      : QStringLiteral("%1 (#%2)").arg(axis.axisName).arg(axis.axisNumber);
            m_resultAxisCombo->addItem(label, axis.axisNumber);
            if (axis.axisNumber == currentAxisNumber) {
                axisIndexToSelect = i;
            }
        }
        if (m_resultAxisCombo->count() > 0) {
            m_resultAxisCombo->setCurrentIndex(axisIndexToSelect);
        }
    }
    populateResultTestSelector(m_resultAxisCombo->currentData().toInt(), currentTestKey);
}

void AxisPerformancePage::populateResultTestSelector(int axisNumber, const QString& preferredTestKey)
{
    QSignalBlocker blocker(m_resultTestCombo);
    m_resultTestCombo->clear();
    auto axisIt = std::find_if(m_resultDocument.axes.constBegin(),
                               m_resultDocument.axes.constEnd(),
                               [axisNumber](const AxisPerformanceImportedAxisResult& axis) {
                                   return axis.axisNumber == axisNumber;
                               });
    if (axisIt == m_resultDocument.axes.constEnd() && m_resultDocument.axes.size() == 1) {
        axisIt = m_resultDocument.axes.constBegin();
    }
    if (axisIt != m_resultDocument.axes.constEnd()) {
        const AxisPerformanceImportedAxisResult& axis = *axisIt;
        int testIndexToSelect = 0;
        for (int i = 0; i < axis.tests.size(); ++i) {
            const AxisPerformanceImportedTestResult& test = axis.tests.at(i);
            const QString label = displayNameForTestKey(test.key);
            m_resultTestCombo->addItem(label, test.key);
            if (!preferredTestKey.isEmpty() && test.key == preferredTestKey) {
                testIndexToSelect = i;
            }
        }
        if (m_resultTestCombo->count() > 0) {
            m_resultTestCombo->setCurrentIndex(testIndexToSelect);
        }
    }
}

void AxisPerformancePage::refreshResultView()
{
    if (!m_resultSummaryLabel || !m_resultMetricTable || !m_chartWidget || !m_reportPreview) {
        return;
    }

    auto* chartWidget = static_cast<AxisPerformanceChartWidget*>(m_chartWidget);

    if (!m_resultDocument.axes.isEmpty() && m_resultAxisCombo && m_resultTestCombo
        && (m_resultAxisCombo->count() == 0 || m_resultTestCombo->count() == 0)) {
        populateResultSelectors();
    }

    if (m_resultDocument.axes.isEmpty()) {
        m_resultSummaryLabel->setText(trText(m_languageCode,
                                             QStringLiteral("Import a sampled result JSON file to calculate axis performance."),
                                             QString::fromUtf8(u8"请导入采样结果 JSON 文件以计算轴性能。")));
        m_resultMetricTable->setRowCount(0);
        chartWidget->setSeries({});
        chartWidget->setXAxisInfo(QStringLiteral("Sample Index"), QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"));
        chartWidget->setYAxisInfo(QString());
        m_reportPreview->clear();
        m_reportPreview->hide();
        return;
    }

    const AxisPerformanceImportedAxisResult* axisPtr = nullptr;
    if (m_resultAxisCombo && m_resultAxisCombo->count() > 0) {
        const int axisNumber = m_resultAxisCombo->currentData().toInt();
        auto axisIt = std::find_if(m_resultDocument.axes.constBegin(),
                                   m_resultDocument.axes.constEnd(),
                                   [axisNumber](const AxisPerformanceImportedAxisResult& axis) {
                                       return axis.axisNumber == axisNumber;
                                   });
        if (axisIt != m_resultDocument.axes.constEnd()) {
            axisPtr = &(*axisIt);
        }
    }

    if (!axisPtr) {
        axisPtr = &m_resultDocument.axes.first();
        if (m_resultAxisCombo && m_resultAxisCombo->count() > 0) {
            const int axisIndex = m_resultAxisCombo->findData(axisPtr->axisNumber);
            if (axisIndex >= 0) {
                QSignalBlocker blocker(m_resultAxisCombo);
                m_resultAxisCombo->setCurrentIndex(axisIndex);
            }
        }
    }

    if (!axisPtr || axisPtr->tests.isEmpty()) {
        m_resultSummaryLabel->setText(trText(m_languageCode,
                                             QStringLiteral("No test result data is available yet."),
                                             QString::fromUtf8(u8"暂时还没有可显示的测试结果数据。")));
        m_resultMetricTable->setRowCount(0);
        chartWidget->setSeries({});
        chartWidget->setXAxisInfo(QStringLiteral("Sample Index"), QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("0"));
        chartWidget->setYAxisInfo(QString());
        m_reportPreview->clear();
        m_reportPreview->hide();
        return;
    }

    if (m_resultTestCombo && m_resultTestCombo->count() == 0 && !axisPtr->tests.isEmpty()) {
        populateResultTestSelector(axisPtr->axisNumber);
    }

    const AxisPerformanceImportedTestResult* testPtr = nullptr;
    if (m_resultTestCombo && m_resultTestCombo->count() > 0) {
        const QString testKey = m_resultTestCombo->currentData().toString();
        auto testIt = std::find_if(axisPtr->tests.constBegin(),
                                   axisPtr->tests.constEnd(),
                                   [&testKey](const AxisPerformanceImportedTestResult& test) {
                                       return test.key == testKey;
                                   });
        if (testIt != axisPtr->tests.constEnd()) {
            testPtr = &(*testIt);
        }
    }

    if (!testPtr) {
        testPtr = &axisPtr->tests.first();
        if (m_resultTestCombo && m_resultTestCombo->count() > 0) {
            const int testIndex = m_resultTestCombo->findData(testPtr->key);
            if (testIndex >= 0) {
                QSignalBlocker blocker(m_resultTestCombo);
                m_resultTestCombo->setCurrentIndex(testIndex);
            }
        }
    }

    const AxisPerformanceImportedAxisResult& axis = *axisPtr;
    const AxisPerformanceImportedTestResult& test = *testPtr;

    m_resultSummaryLabel->setText(trText(m_languageCode,
                                         QStringLiteral("Axis %1 (#%2), test %3, verdict: %4")
                                             .arg(axis.axisName)
                                             .arg(axis.axisNumber)
                                             .arg(displayNameForTestKey(test.key))
                                             .arg(test.verdict),
                                         QString::fromUtf8(u8"\u8f74 %1 (#%2)\uff0c\u6d4b\u8bd5 %3\uff0c\u7ed3\u8bba\uff1a%4")
                                             .arg(axis.axisName)
                                             .arg(axis.axisNumber)
                                             .arg(displayNameForTestKey(test.key))
                                             .arg(test.verdict)));

    {
        QSignalBlocker blocker(m_resultMetricTable);
        m_resultMetricTable->setRowCount(0);
        for (const AxisPerformanceMetric& metric : test.metrics) {
            const int row = m_resultMetricTable->rowCount();
            m_resultMetricTable->insertRow(row);
            m_resultMetricTable->setItem(row, 0, makeItem(metric.name));
            m_resultMetricTable->setItem(row, 1, makeItem(metric.value));
            m_resultMetricTable->setItem(row, 2, makeItem(metric.status));
        }
    }

    chartWidget->setSeries(test.series);
    const ChartAxisInfo axisInfo = buildChartAxisInfo(test);
    chartWidget->setXAxisInfo(axisInfo.xTitle, axisInfo.xLeft, axisInfo.xCenter, axisInfo.xRight);
    chartWidget->setYAxisInfo(axisInfo.yTitle);
    m_reportPreview->setPlainText(buildReportPreviewText(axis, test));
    m_reportPreview->show();
}

QString AxisPerformancePage::buildReportPreviewText(const AxisPerformanceImportedAxisResult& axis,
                                                    const AxisPerformanceImportedTestResult& test) const
{
    QString preview;
    QTextStream out(&preview);
    out.setEncoding(QStringConverter::Utf8);

    out << trText(m_languageCode, QStringLiteral("Axis Performance Report Preview"), QString::fromUtf8(u8"轴性能测试报告预览")) << "\n";
    out << trText(m_languageCode,
                  QStringLiteral("Axis: %1 (#%2)").arg(axis.axisName).arg(axis.axisNumber),
                  QString::fromUtf8(u8"轴：%1 (#%2)").arg(axis.axisName).arg(axis.axisNumber))
        << "\n";
    out << trText(m_languageCode,
                  QStringLiteral("Test: %1").arg(displayNameForTestKey(test.key)),
                  QString::fromUtf8(u8"测试项：%1").arg(displayNameForTestKey(test.key)))
        << "\n";
    out << trText(m_languageCode,
                  QStringLiteral("Verdict: %1").arg(test.verdict),
                  QString::fromUtf8(u8"结论：%1").arg(test.verdict))
        << "\n";

    if (!test.description.isEmpty()) {
        out << trText(m_languageCode,
                      QStringLiteral("Description: %1").arg(test.description),
                      QString::fromUtf8(u8"说明：%1").arg(test.description))
            << "\n";
    }

    if (!test.metrics.isEmpty()) {
        out << "\n" << trText(m_languageCode, QStringLiteral("Metrics"), QString::fromUtf8(u8"指标")) << "\n";
        for (const AxisPerformanceMetric& metric : test.metrics) {
            out << " - " << metric.name << ": " << metric.value;
            if (!metric.status.isEmpty()) {
                out << " [" << metric.status << "]";
            }
            out << "\n";
        }
    }

    const QString executionWarning = test.meta.value(QStringLiteral("executionWarning")).trimmed();
    if (!executionWarning.isEmpty()) {
        out << "\n"
            << trText(m_languageCode, QStringLiteral("Execution Warning"), QString::fromUtf8(u8"执行警告"))
            << ": " << executionWarning << "\n";
    }

    if (!test.series.isEmpty()) {
        out << "\n"
            << trText(m_languageCode,
                      QStringLiteral("Sample curves have been rendered above."),
                      QString::fromUtf8(u8"采样曲线已显示在上方。"))
            << "\n";
    }

    return preview.trimmed();
}

QJsonObject AxisPerformancePage::serializeResultDocument(const AxisPerformanceImportedDocument& document) const
{
    QJsonObject root;
    root.insert(QStringLiteral("sourceFile"), document.sourceFile);

    QJsonArray axisArray;
    for (const AxisPerformanceImportedAxisResult& axis : document.axes) {
        QJsonObject axisObject;
        axisObject.insert(QStringLiteral("axisName"), axis.axisName);
        axisObject.insert(QStringLiteral("axisNumber"), axis.axisNumber);

        QJsonArray testArray;
        for (const AxisPerformanceImportedTestResult& test : axis.tests) {
            QJsonObject testObject;
            testObject.insert(QStringLiteral("key"), test.key);
            testObject.insert(QStringLiteral("name"), test.name);
            testObject.insert(QStringLiteral("description"), test.description);

            QJsonObject metaObject;
            for (auto it = test.meta.constBegin(); it != test.meta.constEnd(); ++it) {
                metaObject.insert(it.key(), it.value());
            }
            testObject.insert(QStringLiteral("meta"), metaObject);

            QJsonArray seriesArray;
            for (const AxisPerformanceSeries& series : test.series) {
                QJsonObject seriesObject;
                seriesObject.insert(QStringLiteral("key"), series.key);
                seriesObject.insert(QStringLiteral("name"), series.name);
                seriesObject.insert(QStringLiteral("unit"), series.unit);
                if (!series.renderStyle.isEmpty()) {
                    seriesObject.insert(QStringLiteral("renderStyle"), series.renderStyle);
                }
                QJsonArray valuesArray;
                for (double value : series.values) {
                    valuesArray.push_back(std::isfinite(value) ? QJsonValue(value) : QJsonValue());
                }
                seriesObject.insert(QStringLiteral("values"), valuesArray);
                seriesArray.push_back(seriesObject);
            }
            testObject.insert(QStringLiteral("series"), seriesArray);
            testArray.push_back(testObject);
        }

        axisObject.insert(QStringLiteral("tests"), testArray);
        axisArray.push_back(axisObject);
    }

    root.insert(QStringLiteral("axisResults"), axisArray);
    return root;
}

AxisPerformanceImportedDocument AxisPerformancePage::parseResultObject(const QJsonObject& object) const
{
    return m_workflowService.parseResultDocument(QJsonDocument(object).toJson(QJsonDocument::Compact), nullptr);
}

void AxisPerformancePage::recomputeDocumentMetrics(AxisPerformanceImportedDocument* document) const
{
    m_workflowService.recomputeDocumentMetrics(document, m_profiles, m_languageCode);
}

QString AxisPerformancePage::buildReportHtml() const
{
    QString html;
    QTextStream out(&html);
    out.setEncoding(QStringConverter::Utf8);
    out << "<html><head><meta charset=\"utf-8\">"
           "<style>"
           "body{font-family:'Segoe UI',sans-serif;margin:24px;color:#21313c;}"
           "h1,h2,h3{color:#18324a;}"
           "table{border-collapse:collapse;width:100%;margin:12px 0 20px 0;}"
           "th,td{border:1px solid #d7dee5;padding:8px 10px;text-align:left;}"
           "th{background:#f4f8fb;}"
           ".meta{color:#5a6b77;margin-bottom:20px;}"
           "</style></head><body>";
    out << "<h1>" << trText(m_languageCode, QStringLiteral("Axis Performance Report"), QString::fromUtf8(u8"轴性能测试报告")) << "</h1>";
    out << "<div class=\"meta\">" << trText(m_languageCode, QStringLiteral("Source file: "), QString::fromUtf8(u8"源文件：")) << m_resultDocument.sourceFile.toHtmlEscaped() << "</div>";

    for (const AxisPerformanceImportedAxisResult& axis : m_resultDocument.axes) {
        out << "<h2>" << (axis.axisName.isEmpty() ? QStringLiteral("Axis") : axis.axisName).toHtmlEscaped()
            << " (#" << axis.axisNumber << ")</h2>";
        for (const AxisPerformanceImportedTestResult& test : axis.tests) {
            out << "<h3>" << displayNameForTestKey(test.key).toHtmlEscaped() << " - " << test.verdict.toHtmlEscaped() << "</h3>";
            if (!test.description.isEmpty()) {
                out << "<div class=\"meta\">" << test.description.toHtmlEscaped() << "</div>";
            }
            out << "<table><tr><th>"
                << trText(m_languageCode, QStringLiteral("Metric"), QString::fromUtf8(u8"\u6307\u6807")).toHtmlEscaped()
                << "</th><th>"
                << trText(m_languageCode, QStringLiteral("Value"), QString::fromUtf8(u8"\u6570\u503c")).toHtmlEscaped()
                << "</th><th>"
                << trText(m_languageCode, QStringLiteral("Verdict"), QString::fromUtf8(u8"\u7ed3\u8bba")).toHtmlEscaped()
                << "</th></tr>";
            for (const AxisPerformanceMetric& metric : test.metrics) {
                out << "<tr><td>" << metric.name.toHtmlEscaped()
                    << "</td><td>" << metric.value.toHtmlEscaped()
                    << "</td><td>" << metric.status.toHtmlEscaped()
                    << "</td></tr>";
            }
            out << "</table>";
            const QString chartImageDataUrl = buildInlineSeriesChartDataUrl(test.series, buildChartAxisInfo(test));
            if (!chartImageDataUrl.isEmpty()) {
                out << "<div style=\"margin:12px 0 20px 0;overflow-x:auto;\">"
                    << "<div style=\"font-weight:600;margin-bottom:8px;\">"
                    << trText(m_languageCode, QStringLiteral("Curve"), QString::fromUtf8(u8"曲线")).toHtmlEscaped()
                    << "</div>"
                    << "<img src=\"" << chartImageDataUrl << "\" style=\"display:block;max-width:100%;height:auto;border-radius:10px;\"/>"
                    << "</div>";
            }
        }
    }

    out << "</body></html>";
    return html;
}

void AxisPerformancePage::mergeLatestResultDocument(const AxisPerformanceImportedDocument& document)
{
    m_workflowService.mergeResultDocuments(&m_resultDocument, document);
}

void AxisPerformancePage::selectResultViewForTest(int axisNumber, const QString& testKey)
{
    if (!m_resultAxisCombo || !m_resultTestCombo) {
        return;
    }

    int axisIndex = -1;
    for (int index = 0; index < m_resultAxisCombo->count(); ++index) {
        if (m_resultAxisCombo->itemData(index).toInt() == axisNumber) {
            axisIndex = index;
            break;
        }
    }
    if (axisIndex >= 0) {
        m_resultAxisCombo->setCurrentIndex(axisIndex);
    }

    int testIndex = -1;
    for (int index = 0; index < m_resultTestCombo->count(); ++index) {
        if (m_resultTestCombo->itemData(index).toString() == testKey) {
            testIndex = index;
            break;
        }
    }
    if (testIndex >= 0) {
        m_resultTestCombo->setCurrentIndex(testIndex);
    }
    refreshResultView();
}

QString AxisPerformancePage::buildResultGuideHtml() const
{
    const QString sampleJson = QStringLiteral(
        "{\n"
        "  \"axisResults\": [\n"
        "    {\n"
        "      \"axisName\": \"X?\",\n"
        "      \"axisNumber\": 0,\n"
        "      \"tests\": [\n"
        "        {\n"
        "        {\n"
        "          \"key\": \"static_jitter\",\n"
        "          \"name\": \"Static Jitter\",\n"
        "          \"meta\": {\n"
        "            \"sampleIntervalMs\": 1,\n"
        "            \"pointCount\": 10\n"
        "          },\n"
        "          \"series\": [\n"
        "            {\n"
        "              \"key\": \"position\",\n"
        "              \"name\": \"Position\",\n"
        "              \"unit\": \"mm\",\n"
        "              \"values\": [0.001, 0.002, 0.0015]\n"
        "            }\n"
        "        }\n"
        "      ]\n"
        "    }\n"
        "  ]\n"
        "}");

    return QStringLiteral(
               "<h3>%1</h3>"
               "<p>%2</p>"
               "<p>%3</p>"
               "<ul>"
               "<li><b>static_jitter</b> - %4</li>"
               "<li><b>settling_time</b> - %5</li>"
               "<li><b>speed_uniformity</b> - %6</li>"
               "<li><b>dynamic_capability</b> - %7</li>"
               "<li><b>power_off_drop_distance</b> - %8</li>"
               "<li><b>limit_window</b> - %9</li>"
               "</ul>"
               "<p>%10</p>"
               "<pre style='background:#f6f8fa;border:1px solid #d7dee5;border-radius:8px;padding:10px;white-space:pre-wrap;'>%11</pre>")
        .arg(trText(m_languageCode, QStringLiteral("Result Format Guide"), QString::fromUtf8(u8"结果格式说明")))
        .arg(trText(m_languageCode,
                    QStringLiteral("Import a sampled JSON file generated from your axis performance buffers. Each axis contains multiple tests, and each test contains one or more sampled series."),
                    QString::fromUtf8(u8"请导入由轴性能 buffer 导出的采样 JSON 文件。每个轴可以包含多个测试，每个测试包含一组或多组采样序列。")))
        .arg(trText(m_languageCode,
                    QStringLiteral("A ready-to-fill template file is available at Exports/axis_performance_result_template.json."),
                    QString::fromUtf8(u8"可参考 Exports/axis_performance_result_template.json 中的模板文件。")))
        .arg(trText(m_languageCode, QStringLiteral("position-only jitter samples"), QString::fromUtf8(u8"仅位置抖动采样")))
        .arg(trText(m_languageCode, QStringLiteral("position + AST.INPOS + MST.INPOS"), QString::fromUtf8(u8"位置 + AST.INPOS + MST.INPOS")))
        .arg(trText(m_languageCode, QStringLiteral("velocity samples at multiple commanded speeds"), QString::fromUtf8(u8"多个指令速度下的速度采样")))
        .arg(trText(m_languageCode, QStringLiteral("position/velocity/acceleration/jerk samples"), QString::fromUtf8(u8"位置/速度/加速度/加加速度采样")))
        .arg(trText(m_languageCode, QStringLiteral("drop distance before and after disable"), QString::fromUtf8(u8"失能前后的下落距离")))
        .arg(trText(m_languageCode, QStringLiteral("electrical and software limit results"), QString::fromUtf8(u8"电气限位与软件限位结果")))
        .arg(trText(m_languageCode, QStringLiteral("Recommended minimal JSON example:"), QString::fromUtf8(u8"推荐的最小 JSON 示例：")))
        .arg(sampleJson.toHtmlEscaped());
}

QString AxisPerformancePage::verdictForThreshold(bool passed) const
{
    return passed
               ? trText(m_languageCode, QStringLiteral("PASS"), QStringLiteral("PASS"))
               : trText(m_languageCode, QStringLiteral("FAIL"), QStringLiteral("FAIL"));
}

void AxisPerformancePage::buildUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(12);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_editConfigButton = new QPushButton(this);
    m_editConfigButton->setStyleSheet(UiHelpers::secondaryButtonStyle());
    auto* topRowLayout = new QHBoxLayout();
    topRowLayout->setContentsMargins(0, 0, 0, 0);
    topRowLayout->setSpacing(10);
    topRowLayout->addWidget(m_statusLabel, 1);
    topRowLayout->addWidget(m_editConfigButton, 0, Qt::AlignTop);

    auto* executionCard = new QFrame(this);
    executionCard->setStyleSheet(UiHelpers::cardStyle());
    auto* executionLayout = new QVBoxLayout(executionCard);
    executionLayout->setContentsMargins(14, 14, 14, 14);
    executionLayout->setSpacing(10);

    auto* executionHeader = new QHBoxLayout();
    auto* executionTitle = new QLabel(trText(m_languageCode, QStringLiteral("Test Items"), QString::fromUtf8(u8"测试项目")), executionCard);
    executionTitle->setStyleSheet(QStringLiteral("font-size:16px;font-weight:700;color:#18324a;"));
    auto* executionAxisLabel = new QLabel(executionCard);
    executionAxisLabel->setObjectName(QStringLiteral("executionAxisLabel"));
    auto* executionTestLabel = new QLabel(executionCard);
    executionTestLabel->setObjectName(QStringLiteral("executionTestLabel"));
    m_executionAxisCombo = new QComboBox(executionCard);
    m_executionTestCombo = new QComboBox(executionCard);
    m_executionTestCombo->setMinimumWidth(360);
    m_runSelectedTestButton = new QPushButton(executionCard);
    m_openSelectedScriptButton = new QPushButton(executionCard);
    executionHeader->addWidget(executionTitle);
    executionHeader->addStretch();
    executionHeader->addWidget(executionAxisLabel);
    executionHeader->addWidget(m_executionAxisCombo);
    executionHeader->addWidget(executionTestLabel);
    executionHeader->addWidget(m_executionTestCombo);
    executionHeader->addWidget(m_runSelectedTestButton);
    executionHeader->addWidget(m_openSelectedScriptButton);

    executionLayout->addLayout(executionHeader);

    m_analysisCard = new QFrame(this);
    m_analysisCard->setStyleSheet(UiHelpers::cardStyle());
    auto* analysisLayout = new QVBoxLayout(m_analysisCard);
    analysisLayout->setContentsMargins(14, 14, 14, 14);
    analysisLayout->setSpacing(10);

    auto* analysisHeaderLayout = new QHBoxLayout();
    analysisHeaderLayout->setContentsMargins(0, 0, 0, 0);
    auto* analysisTitle = new QLabel(trText(m_languageCode, QStringLiteral("Test Report"), QString::fromUtf8(u8"测试报告")), m_analysisCard);
    analysisTitle->setStyleSheet(QStringLiteral("font-size:16px;font-weight:700;color:#18324a;"));
    m_importResultsButton = new QPushButton(m_analysisCard);
    m_exportReportButton = new QPushButton(m_analysisCard);
    m_importResultsButton->setStyleSheet(UiHelpers::secondaryButtonStyle());
    m_exportReportButton->setStyleSheet(UiHelpers::primaryButtonStyle());
    analysisHeaderLayout->addWidget(analysisTitle);
    analysisHeaderLayout->addStretch();
    analysisHeaderLayout->addWidget(m_importResultsButton);
    analysisHeaderLayout->addWidget(m_exportReportButton);

    auto* selectorLayout = new QHBoxLayout();
    selectorLayout->setContentsMargins(0, 0, 0, 0);
    auto* axisLabel = new QLabel(m_analysisCard);
    axisLabel->setObjectName(QStringLiteral("resultAxisLabel"));
    auto* testLabel = new QLabel(m_analysisCard);
    testLabel->setObjectName(QStringLiteral("resultTestLabel"));
    m_resultAxisCombo = new QComboBox(m_analysisCard);
    m_resultTestCombo = new QComboBox(m_analysisCard);
    selectorLayout->addWidget(axisLabel);
    selectorLayout->addWidget(m_resultAxisCombo, 1);
    selectorLayout->addSpacing(12);
    selectorLayout->addWidget(testLabel);
    selectorLayout->addWidget(m_resultTestCombo, 1);

    m_resultSummaryLabel = new QLabel(m_analysisCard);
    m_resultSummaryLabel->setWordWrap(true);
    m_resultSummaryLabel->setStyleSheet(QStringLiteral("padding:8px 10px;border:1px solid #d8e0e8;border-radius:8px;background:#f8fbfd;color:#32424f;"));

    m_resultMetricTable = new QTableWidget(m_analysisCard);
    m_resultMetricTable->setColumnCount(3);
    m_resultMetricTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_resultMetricTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultMetricTable->verticalHeader()->setVisible(false);
    m_resultMetricTable->horizontalHeader()->setStretchLastSection(true);
    m_resultMetricTable->setMinimumHeight(180);

    auto* chartTitle = new QLabel(trText(m_languageCode, QStringLiteral("Sample Curves"), QString::fromUtf8(u8"\u91C7\u6837\u66F2\u7EBF")), m_analysisCard);
    chartTitle->setStyleSheet(QStringLiteral("font-size:15px;font-weight:700;color:#18324a;"));
    m_chartWidget = new AxisPerformanceChartWidget(m_analysisCard);

    m_reportPreview = new QTextEdit(m_analysisCard);
    m_reportPreview->setReadOnly(true);
    m_reportPreview->setMinimumHeight(150);

    analysisLayout->addLayout(analysisHeaderLayout);
    analysisLayout->addLayout(selectorLayout);
    analysisLayout->addWidget(m_resultSummaryLabel);
    analysisLayout->addWidget(m_resultMetricTable);
    analysisLayout->addWidget(chartTitle);
    analysisLayout->addWidget(m_chartWidget);
    analysisLayout->addWidget(m_reportPreview);

    m_runtimeLogLabel = new QLabel(this);
    m_runtimeLogLabel->hide();
    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMinimumHeight(180);
    m_logView->setMaximumHeight(0);
    m_logView->setStyleSheet(UiHelpers::logViewStyle());
    m_logView->hide();
    bindFeedbackWidgets(m_statusLabel, m_logView);

    rootLayout->addLayout(topRowLayout);
    rootLayout->addWidget(executionCard);
    rootLayout->addWidget(m_analysisCard, 1);

    connect(m_editConfigButton, &QPushButton::clicked, this, &AxisPerformancePage::openConfigEditorDialog);
    connect(m_importResultsButton, &QPushButton::clicked, this, &AxisPerformancePage::importResultFile);
    connect(m_exportReportButton, &QPushButton::clicked, this, &AxisPerformancePage::exportReport);
    connect(m_executionAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AxisPerformancePage::handleExecutionAxisChanged);
    connect(m_executionTestCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AxisPerformancePage::handleExecutionSelectionChanged);
    connect(m_runSelectedTestButton, &QPushButton::clicked, this, &AxisPerformancePage::runTestItem);
    connect(m_openSelectedScriptButton, &QPushButton::clicked, this, &AxisPerformancePage::openTestScript);
    connect(m_resultAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AxisPerformancePage::handleResultAxisChanged);
    connect(m_resultTestCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AxisPerformancePage::handleResultTestChanged);
    connect(m_configWatcher, &QFileSystemWatcher::fileChanged, this, &AxisPerformancePage::handleConfigFileChanged);

    retranslateUi();
    setReportPanelVisible(true);
}

void AxisPerformancePage::buildConfigEditorDialog()
{
    m_editorDialog = new QDialog(this);
    m_editorDialog->setModal(false);
    m_editorDialog->resize(1300, 820);

    auto* rootLayout = new QVBoxLayout(m_editorDialog);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    m_editorTitleLabel = new QLabel(m_editorDialog);
    m_editorTitleLabel->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#18324a;"));
    m_editorPathLabel = new QLabel(m_editorDialog);
    m_editorBrowseButton = new QPushButton(m_editorDialog);
    m_editorSaveButton = new QPushButton(m_editorDialog);

    auto* pathRow = new QHBoxLayout();
    pathRow->addWidget(m_editorPathLabel, 1);
    pathRow->addWidget(m_editorBrowseButton);

    auto* profileCard = new QFrame(m_editorDialog);
    profileCard->setStyleSheet(QStringLiteral("QFrame{background:#ffffff;border:1px solid #d7dee5;border-radius:12px;}"));
    auto* profileLayout = new QGridLayout(profileCard);
    profileLayout->setContentsMargins(14, 14, 14, 14);
    profileLayout->setHorizontalSpacing(10);
    profileLayout->setVerticalSpacing(10);

    m_editorProfileCombo = new QComboBox(profileCard);
    m_addProfileButton = new QPushButton(profileCard);
    m_removeProfileButton = new QPushButton(profileCard);
    m_profileNameEdit = new QLineEdit(profileCard);
    m_profileDescriptionEdit = new QLineEdit(profileCard);

    auto* profileSelectLabel = new QLabel(profileCard);
    profileSelectLabel->setObjectName(QStringLiteral("profileSelectLabel"));
    auto* profileNameLabel = new QLabel(profileCard);
    profileNameLabel->setObjectName(QStringLiteral("profileNameLabel"));
    auto* profileDescriptionLabel = new QLabel(profileCard);
    profileDescriptionLabel->setObjectName(QStringLiteral("profileDescriptionLabel"));

    profileLayout->addWidget(profileSelectLabel, 0, 0);
    profileLayout->addWidget(m_editorProfileCombo, 0, 1, 1, 2);
    profileLayout->addWidget(m_addProfileButton, 0, 3);
    profileLayout->addWidget(m_removeProfileButton, 0, 4);
    profileLayout->addWidget(profileNameLabel, 1, 0);
    profileLayout->addWidget(m_profileNameEdit, 1, 1, 1, 4);
    profileLayout->addWidget(profileDescriptionLabel, 2, 0);
    profileLayout->addWidget(m_profileDescriptionEdit, 2, 1, 1, 4);

    auto* centerLayout = new QHBoxLayout();
    centerLayout->setSpacing(12);

    auto* axisCard = new QFrame(m_editorDialog);
    axisCard->setStyleSheet(QStringLiteral("QFrame{background:#ffffff;border:1px solid #d7dee5;border-radius:12px;}"));
    axisCard->setMinimumWidth(320);
    auto* axisLayout = new QVBoxLayout(axisCard);
    axisLayout->setContentsMargins(14, 14, 14, 14);
    axisLayout->setSpacing(10);

    auto* axisHeader = new QHBoxLayout();
    auto* axisTitleLabel = new QLabel(axisCard);
    axisTitleLabel->setObjectName(QStringLiteral("axisTitleLabel"));
    axisHeader->addWidget(axisTitleLabel);
    axisHeader->addStretch();
    m_addAxisButton = new QPushButton(axisCard);
    m_removeAxisButton = new QPushButton(axisCard);
    axisHeader->addWidget(m_addAxisButton);
    axisHeader->addWidget(m_removeAxisButton);

    m_axisListWidget = new QListWidget(axisCard);
    m_axisListWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* axisForm = new QFormLayout();
    axisForm->setContentsMargins(0, 0, 0, 0);
    axisForm->setSpacing(8);
    auto* axisNameLabel = new QLabel(axisCard);
    axisNameLabel->setObjectName(QStringLiteral("axisNameLabel"));
    auto* axisNumberLabel = new QLabel(axisCard);
    axisNumberLabel->setObjectName(QStringLiteral("axisNumberLabel"));
    m_axisNameEdit = new QLineEdit(axisCard);
    m_axisNumberEdit = new QLineEdit(axisCard);
    m_axisNumberEdit->setValidator(new QDoubleValidator(0, 9999, 0, m_axisNumberEdit));
    axisForm->addRow(axisNameLabel, m_axisNameEdit);
    axisForm->addRow(axisNumberLabel, m_axisNumberEdit);

    axisLayout->addLayout(axisHeader);
    axisLayout->addWidget(m_axisListWidget, 1);
    axisLayout->addLayout(axisForm);

    auto* detailLayout = new QVBoxLayout();
    detailLayout->setSpacing(12);

    auto* testCard = new QFrame(m_editorDialog);
    testCard->setStyleSheet(QStringLiteral("QFrame{background:#ffffff;border:1px solid #d7dee5;border-radius:12px;}"));
    auto* testLayout = new QVBoxLayout(testCard);
    testLayout->setContentsMargins(14, 14, 14, 14);
    testLayout->setSpacing(10);
    auto* testHeader = new QHBoxLayout();
    auto* testTitleLabel = new QLabel(testCard);
    testTitleLabel->setObjectName(QStringLiteral("testTitleLabel"));
    testHeader->addWidget(testTitleLabel);
    m_testTemplateCombo = new QComboBox(testCard);
    testHeader->addWidget(m_testTemplateCombo);
    testHeader->addStretch();
    m_addTestButton = new QPushButton(testCard);
    m_removeTestButton = new QPushButton(testCard);
    testHeader->addWidget(m_addTestButton);
    testHeader->addWidget(m_removeTestButton);
    m_testTable = new QTableWidget(testCard);
    m_testTable->setColumnCount(TestColumnCount);
    m_testTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_testTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_testTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_testTable->verticalHeader()->setVisible(false);
    m_testTable->horizontalHeader()->setStretchLastSection(true);
    testLayout->addLayout(testHeader);
    testLayout->addWidget(m_testTable, 1);

    auto* parameterCard = new QFrame(m_editorDialog);
    parameterCard->setStyleSheet(QStringLiteral("QFrame{background:#ffffff;border:1px solid #d7dee5;border-radius:12px;}"));
    auto* parameterLayout = new QVBoxLayout(parameterCard);
    parameterLayout->setContentsMargins(14, 14, 14, 14);
    parameterLayout->setSpacing(10);
    auto* parameterHeader = new QHBoxLayout();
    auto* parameterTitleLabel = new QLabel(parameterCard);
    parameterTitleLabel->setObjectName(QStringLiteral("parameterTitleLabel"));
    parameterHeader->addWidget(parameterTitleLabel);
    m_parameterSetCombo = new QComboBox(parameterCard);
    parameterHeader->addWidget(m_parameterSetCombo);
    m_addParameterSetButton = new QPushButton(parameterCard);
    m_removeParameterSetButton = new QPushButton(parameterCard);
    parameterHeader->addWidget(m_addParameterSetButton);
    parameterHeader->addWidget(m_removeParameterSetButton);
    parameterHeader->addStretch();
    m_addParameterButton = new QPushButton(parameterCard);
    m_removeParameterButton = new QPushButton(parameterCard);
    m_addParameterButton->setVisible(false);
    m_removeParameterButton->setVisible(false);
    parameterHeader->addWidget(m_addParameterButton);
    parameterHeader->addWidget(m_removeParameterButton);
    m_parameterTable = new QTableWidget(parameterCard);
    m_parameterTable->setColumnCount(ParameterColumnCount);
    m_parameterTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_parameterTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_parameterTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    m_parameterTable->verticalHeader()->setVisible(false);
    m_parameterTable->horizontalHeader()->setStretchLastSection(true);
    parameterLayout->addLayout(parameterHeader);
    parameterLayout->addWidget(m_parameterTable, 1);

    detailLayout->addWidget(testCard, 1);
    detailLayout->addWidget(parameterCard, 1);

    centerLayout->addWidget(axisCard);
    centerLayout->addLayout(detailLayout, 1);

    auto* saveRow = new QHBoxLayout();
    saveRow->addStretch();
    saveRow->addWidget(m_editorSaveButton);

    rootLayout->addWidget(m_editorTitleLabel);
    rootLayout->addLayout(pathRow);
    rootLayout->addWidget(profileCard);
    rootLayout->addLayout(centerLayout, 1);
    rootLayout->addLayout(saveRow);

    connect(m_editorBrowseButton, &QPushButton::clicked, this, &AxisPerformancePage::browseConfigFile);
    connect(m_editorSaveButton, &QPushButton::clicked, this, &AxisPerformancePage::saveConfig);

    connect(m_editorProfileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AxisPerformancePage::handleEditorProfileChanged);
    connect(m_profileNameEdit, &QLineEdit::textEdited, this, &AxisPerformancePage::handleProfileNameEdited);
    connect(m_profileDescriptionEdit, &QLineEdit::textEdited, this, &AxisPerformancePage::handleProfileDescriptionEdited);
    connect(m_addProfileButton, &QPushButton::clicked, this, &AxisPerformancePage::addProfile);
    connect(m_removeProfileButton, &QPushButton::clicked, this, &AxisPerformancePage::removeProfile);

    connect(m_axisListWidget, &QListWidget::currentRowChanged, this, &AxisPerformancePage::handleAxisSelectionChanged);
    connect(m_axisNameEdit, &QLineEdit::textEdited, this, &AxisPerformancePage::handleAxisNameEdited);
    connect(m_axisNumberEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        bool ok = false;
        const int value = text.toInt(&ok);
        if (ok) {
            handleAxisNumberEdited(value);
        }
    });
    connect(m_addAxisButton, &QPushButton::clicked, this, &AxisPerformancePage::addAxis);
    connect(m_removeAxisButton, &QPushButton::clicked, this, &AxisPerformancePage::removeAxis);

    connect(m_testTable, &QTableWidget::itemSelectionChanged, this, &AxisPerformancePage::handleTestSelectionChanged);
    connect(m_addTestButton, &QPushButton::clicked, this, &AxisPerformancePage::addTestItem);
    connect(m_removeTestButton, &QPushButton::clicked, this, &AxisPerformancePage::removeTestItem);

    connect(m_parameterTable, &QTableWidget::itemChanged, this, &AxisPerformancePage::handleParameterItemChanged);
    connect(m_parameterSetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AxisPerformancePage::handleParameterSetChanged);
    connect(m_addParameterSetButton, &QPushButton::clicked, this, &AxisPerformancePage::addParameterSet);
    connect(m_removeParameterSetButton, &QPushButton::clicked, this, &AxisPerformancePage::removeParameterSet);

    retranslateUi();
}

void AxisPerformancePage::retranslateUi()
{
    if (m_editConfigButton) m_editConfigButton->setText(textForKey(QStringLiteral("settings")));
    if (m_importResultsButton) m_importResultsButton->setText(trText(m_languageCode, QStringLiteral("Import Results"), QString::fromUtf8(u8"导入结果")));
    if (m_exportReportButton) m_exportReportButton->setText(trText(m_languageCode, QStringLiteral("Export Report"), QString::fromUtf8(u8"导出报告")));
    if (m_runtimeLogLabel) m_runtimeLogLabel->clear();
    if (m_resultMetricTable) {
        m_resultMetricTable->setHorizontalHeaderLabels(
            {trText(m_languageCode, QStringLiteral("Metric"), QString::fromUtf8(u8"\u6307\u6807")),
             trText(m_languageCode, QStringLiteral("Value"), QString::fromUtf8(u8"\u6570\u503c")),
             trText(m_languageCode, QStringLiteral("Verdict"), QString::fromUtf8(u8"\u7ed3\u8bba"))});
    }
    if (m_reportPreview && m_reportPreview->toPlainText().isEmpty()) {
        m_reportPreview->setPlainText(trText(m_languageCode,
                                             QStringLiteral("Import sampled result JSON to calculate axis performance and preview the report here."),
                                             QString::fromUtf8(u8"\u5bfc\u5165\u91c7\u6837\u7ed3\u679c JSON \u540e\uff0c\u53ef\u5728\u6b64\u8ba1\u7b97\u8f74\u6027\u80fd\u5e76\u9884\u89c8\u62a5\u544a\u3002")));
    }

    if (m_editorDialog) m_editorDialog->setWindowTitle(textForKey(QStringLiteral("editor_title")));
    if (m_editorTitleLabel) m_editorTitleLabel->setText(textForKey(QStringLiteral("editor_title")));
    if (m_editorPathLabel) {
        m_editorPathLabel->setText(trText(m_languageCode,
                                          QStringLiteral("Config: %1").arg(configFilePath()),
                                          QString::fromUtf8(u8"配置文件：%1").arg(configFilePath())));
    }
    if (m_editorBrowseButton) m_editorBrowseButton->setText(textForKey(QStringLiteral("browse")));
    if (m_editorSaveButton) m_editorSaveButton->setText(textForKey(QStringLiteral("save")));
    if (m_addProfileButton) m_addProfileButton->setText(textForKey(QStringLiteral("add_profile")));
    if (m_removeProfileButton) m_removeProfileButton->setText(textForKey(QStringLiteral("remove_profile")));
    if (m_addAxisButton) m_addAxisButton->setText(textForKey(QStringLiteral("add_axis")));
    if (m_removeAxisButton) m_removeAxisButton->setText(textForKey(QStringLiteral("remove_axis")));
    if (m_addTestButton) m_addTestButton->setText(textForKey(QStringLiteral("add_test")));
    if (m_removeTestButton) m_removeTestButton->setText(textForKey(QStringLiteral("remove_test")));
    if (m_addParameterSetButton) m_addParameterSetButton->setText(textForKey(QStringLiteral("add_param_set")));
    if (m_removeParameterSetButton) m_removeParameterSetButton->setText(textForKey(QStringLiteral("remove_param_set")));

    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("profileSelectLabel")) : nullptr) label->setText(textForKey(QStringLiteral("profile_select")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("profileNameLabel")) : nullptr) label->setText(textForKey(QStringLiteral("profile_name")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("profileDescriptionLabel")) : nullptr) label->setText(textForKey(QStringLiteral("profile_desc")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("axisTitleLabel")) : nullptr) label->setText(textForKey(QStringLiteral("axes_tab")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("axisNameLabel")) : nullptr) label->setText(textForKey(QStringLiteral("axis_name")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("axisNumberLabel")) : nullptr) label->setText(textForKey(QStringLiteral("axis_number")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("testTitleLabel")) : nullptr) label->setText(textForKey(QStringLiteral("tests_tab")));
    if (QLabel* label = m_editorDialog ? m_editorDialog->findChild<QLabel*>(QStringLiteral("parameterTitleLabel")) : nullptr) label->setText(textForKey(QStringLiteral("parameters_tab")));

    if (m_testTable) {
        m_testTable->setHorizontalHeaderLabels(
            {textForKey(QStringLiteral("test_name")),
             textForKey(QStringLiteral("test_desc"))});
    }

    if (m_parameterTable) {
        m_parameterTable->setHorizontalHeaderLabels(
            {textForKey(QStringLiteral("param_name")),
             textForKey(QStringLiteral("param_value")),
             textForKey(QStringLiteral("unit"))});
    }

    if (m_statusLabel && m_statusLabel->text().isEmpty()) {
        m_statusLabel->setText(textForKey(QStringLiteral("ready")));
        m_statusLabel->setStyleSheet(UiHelpers::statusStyle(QStringLiteral("info")));
    }

    if (m_importResultsButton && m_languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) != 0) {
        m_importResultsButton->setText(QString::fromUtf8(u8"\u5BFC\u5165\u7ED3\u679C"));
    }
    if (m_exportReportButton && m_languageCode.compare(QStringLiteral("en-US"), Qt::CaseInsensitive) != 0) {
        m_exportReportButton->setText(QString::fromUtf8(u8"\u5BFC\u51FA\u62A5\u544A"));
    }
    if (QLabel* label = findChild<QLabel*>(QStringLiteral("executionAxisLabel"))) {
        label->setText(trText(m_languageCode, QStringLiteral("Axis"), QString::fromUtf8(u8"\u8F74")));
    }
    if (QLabel* label = findChild<QLabel*>(QStringLiteral("resultAxisLabel"))) {
        label->setText(trText(m_languageCode, QStringLiteral("Axis"), QString::fromUtf8(u8"\u8F74")));
    }
    if (QLabel* label = findChild<QLabel*>(QStringLiteral("resultTestLabel"))) {
        label->setText(trText(m_languageCode, QStringLiteral("Test"), QString::fromUtf8(u8"测试")));
    }
    if (QLabel* label = findChild<QLabel*>(QStringLiteral("executionTestLabel"))) {
        label->setText(trText(m_languageCode, QStringLiteral("Test"), QString::fromUtf8(u8"测试项")));
    }
    if (m_runSelectedTestButton) {
        m_runSelectedTestButton->setText(trText(m_languageCode, QStringLiteral("Run Test"), QString::fromUtf8(u8"\u8fd0\u884c\u6d4b\u8bd5")));
    }
    if (m_openSelectedScriptButton) {
        m_openSelectedScriptButton->setText(trText(m_languageCode, QStringLiteral("Open Script"), QString::fromUtf8(u8"打开脚本")));
    }
    if (m_resultMetricTable) {
        m_resultMetricTable->setHorizontalHeaderLabels(
            {trText(m_languageCode, QStringLiteral("Metric"), QString::fromUtf8(u8"\u6307\u6807")),
             trText(m_languageCode, QStringLiteral("Value"), QString::fromUtf8(u8"\u6570\u503c")),
             trText(m_languageCode, QStringLiteral("Verdict"), QString::fromUtf8(u8"\u7ed3\u8bba"))});
    }

    refreshEditorProfileSection();
    refreshExecutionTable();
    refreshResultView();
}

void AxisPerformancePage::updateWatcher()
{
    const QString path = configFilePath();
    const QStringList files = m_configWatcher->files();
    if (!files.isEmpty()) {
        m_configWatcher->removePaths(files);
    }
    if (!path.isEmpty()) {
        m_configWatcher->addPath(path);
    }
}

void AxisPerformancePage::showStatusMessage(const QString& text, bool isError)
{
    setStatusMessage(text, isError, true);
}

void AxisPerformancePage::logMessage(const QString& message)
{
    appendLogMessage(message);
}

void AxisPerformancePage::loadEditorFromProfiles()
{
    QString currentProfile;
    if (m_editorProfileCombo && m_editorProfileCombo->currentIndex() >= 0) {
        currentProfile = m_editorProfileCombo->currentText();
    }

    m_editorProfiles = m_profiles;
    refreshEditor();

    if (m_editorProfileCombo) {
        int index = currentProfile.isEmpty() ? 0 : m_editorProfileCombo->findText(currentProfile);
        if (index < 0) {
            index = 0;
        }
        if (m_editorProfileCombo->count() > 0) {
            m_editorProfileCombo->setCurrentIndex(index);
        }
    }
}

void AxisPerformancePage::refreshEditor()
{
    m_isRefreshingEditor = true;

    const int currentProfileIndex = m_editorProfileCombo ? m_editorProfileCombo->currentIndex() : 0;
    if (m_editorProfileCombo) {
        QSignalBlocker blocker(m_editorProfileCombo);
        m_editorProfileCombo->clear();
        for (const AxisPerformanceProfile& profile : m_editorProfiles) {
            m_editorProfileCombo->addItem(profile.profileName.isEmpty() ? textForKey(QStringLiteral("unnamed_profile")) : profile.profileName);
        }
        if (m_editorProfileCombo->count() > 0) {
            m_editorProfileCombo->setCurrentIndex(qBound(0, currentProfileIndex, m_editorProfileCombo->count() - 1));
        }
    }

    m_isRefreshingEditor = false;
    refreshEditorProfileSection();
    refreshAxisList();
}

void AxisPerformancePage::refreshEditorProfileSection()
{
    const AxisPerformanceProfile* profile = currentEditorProfile();
    const bool hasProfile = profile != nullptr;

    if (m_profileNameEdit) {
        QSignalBlocker blocker(m_profileNameEdit);
        m_profileNameEdit->setText(hasProfile ? profile->profileName : QString());
        m_profileNameEdit->setEnabled(hasProfile);
    }

    if (m_profileDescriptionEdit) {
        QSignalBlocker blocker(m_profileDescriptionEdit);
        m_profileDescriptionEdit->setText(hasProfile ? profile->description : QString());
        m_profileDescriptionEdit->setEnabled(hasProfile);
    }

    if (m_removeProfileButton) {
        m_removeProfileButton->setEnabled(hasProfile);
    }
}

void AxisPerformancePage::refreshAxisList()
{
    m_isRefreshingEditor = true;

    const AxisPerformanceProfile* profile = currentEditorProfile();
    const int currentAxisIndex = m_axisListWidget ? m_axisListWidget->currentRow() : 0;
    if (m_axisListWidget) {
        QSignalBlocker blocker(m_axisListWidget);
        m_axisListWidget->clear();
        if (profile) {
            for (const AxisPerformanceAxis& axis : profile->axes) {
                m_axisListWidget->addItem(axisSummaryText(axis));
            }
        }
        if (m_axisListWidget->count() > 0) {
            m_axisListWidget->setCurrentRow(qBound(0, currentAxisIndex, m_axisListWidget->count() - 1));
        }
    }

    m_isRefreshingEditor = false;
    refreshAxisDetail();
}

void AxisPerformancePage::refreshAxisDetail()
{
    const AxisPerformanceAxis* axis = currentEditorAxis();
    const bool hasAxis = axis != nullptr;

    if (m_axisNameEdit) {
        QSignalBlocker blocker(m_axisNameEdit);
        m_axisNameEdit->setText(hasAxis ? axis->axisName : QString());
        m_axisNameEdit->setEnabled(hasAxis);
    }

    if (m_axisNumberEdit) {
        QSignalBlocker blocker(m_axisNumberEdit);
        m_axisNumberEdit->setText(hasAxis ? QString::number(axis->axisNumber) : QString());
        m_axisNumberEdit->setEnabled(hasAxis);
    }

    if (m_addAxisButton) {
        m_addAxisButton->setEnabled(currentEditorProfile() != nullptr);
    }
    if (m_removeAxisButton) {
        m_removeAxisButton->setEnabled(hasAxis);
    }

    refreshTestTable();
}

void AxisPerformancePage::refreshTestTable()
{
    m_isRefreshingEditor = true;

    const AxisPerformanceAxis* axis = currentEditorAxis();
    const QVector<AxisPerformanceTestItem> templates = defaultTestTemplates();

    if (m_testTemplateCombo) {
        const QString currentKey = m_testTemplateCombo->currentData().toString();
        QSignalBlocker blocker(m_testTemplateCombo);
        m_testTemplateCombo->clear();
        for (const AxisPerformanceTestItem& templateItem : templates) {
            if (!axis || !axisHasTestKey(*axis, templateItem.key)) {
                m_testTemplateCombo->addItem(templateItem.name, templateItem.key);
            }
        }
        if (m_testTemplateCombo->count() > 0) {
            const int index = currentKey.isEmpty() ? 0 : m_testTemplateCombo->findData(currentKey);
            m_testTemplateCombo->setCurrentIndex(index >= 0 ? index : 0);
        }
        m_testTemplateCombo->setEnabled(axis != nullptr && m_testTemplateCombo->count() > 0);
    }

    const int currentTestIndex = m_testTable ? m_testTable->currentRow() : 0;
    if (m_testTable) {
        QSignalBlocker blocker(m_testTable);
        m_testTable->setRowCount(0);
        if (axis) {
            for (const AxisPerformanceTestItem& testItem : axis->testItems) {
                const int row = m_testTable->rowCount();
                m_testTable->insertRow(row);

                auto* nameItem = makeItem(displayNameForTestKey(testItem.key));
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                auto* descItem = makeItem(testItem.description);
                descItem->setFlags(descItem->flags() & ~Qt::ItemIsEditable);

                m_testTable->setItem(row, TestNameColumn, nameItem);
                m_testTable->setItem(row, TestDescriptionColumn, descItem);
            }
        }
        if (m_testTable->rowCount() > 0) {
            m_testTable->setCurrentCell(qBound(0, currentTestIndex, m_testTable->rowCount() - 1), 0);
        }
    }

    m_isRefreshingEditor = false;

    if (m_addTestButton) {
        m_addTestButton->setEnabled(axis != nullptr && m_testTemplateCombo && m_testTemplateCombo->count() > 0);
    }
    if (m_removeTestButton) {
        m_removeTestButton->setEnabled(axis != nullptr && currentEditorTestIndex() >= 0);
    }

    refreshParameterSetSection();
    refreshParameterTable();
}

void AxisPerformancePage::refreshParameterSetSection()
{
    m_isRefreshingEditor = true;

    const AxisPerformanceTestItem* testItem = currentEditorTestItem();
    const bool showParameterSets = testItem && supportsParameterSets(*testItem);
    const int currentIndex = m_parameterSetCombo ? m_parameterSetCombo->currentIndex() : 0;

    if (m_parameterSetCombo) {
        QSignalBlocker blocker(m_parameterSetCombo);
        m_parameterSetCombo->clear();
        if (showParameterSets) {
            for (int index = 0; index < testItem->parameterSets.size(); ++index) {
                const AxisPerformanceParameterSet& parameterSet = testItem->parameterSets.at(index);
                m_parameterSetCombo->addItem(parameterSetDisplayName(parameterSet, index));
            }
            if (m_parameterSetCombo->count() > 0) {
                m_parameterSetCombo->setCurrentIndex(qBound(0, currentIndex, m_parameterSetCombo->count() - 1));
            }
        }
        m_parameterSetCombo->setVisible(showParameterSets);
        m_parameterSetCombo->setEnabled(showParameterSets && m_parameterSetCombo->count() > 0);
    }

    if (m_addParameterSetButton) {
        m_addParameterSetButton->setVisible(showParameterSets);
        m_addParameterSetButton->setEnabled(showParameterSets);
    }

    if (m_removeParameterSetButton) {
        m_removeParameterSetButton->setVisible(showParameterSets);
        m_removeParameterSetButton->setEnabled(showParameterSets && testItem->parameterSets.size() > 1);
    }

    m_isRefreshingEditor = false;
}

void AxisPerformancePage::refreshParameterTable()
{
    m_isRefreshingEditor = true;

    const QVector<AxisPerformanceParameter>* parameters = currentEditorParameters();
    if (m_parameterTable) {
        QSignalBlocker blocker(m_parameterTable);
        m_parameterTable->setRowCount(0);
        if (parameters) {
            for (const AxisPerformanceParameter& parameter : *parameters) {
                const int row = m_parameterTable->rowCount();
                m_parameterTable->insertRow(row);

                auto* nameItem = makeItem(parameterDisplayName(parameter));
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                auto* valueItem = makeItem(parameter.value);
                auto* unitItem = makeItem(parameter.unit);
                unitItem->setFlags(unitItem->flags() & ~Qt::ItemIsEditable);

                m_parameterTable->setItem(row, ParameterNameColumn, nameItem);
                m_parameterTable->setItem(row, ParameterValueColumn, valueItem);
                m_parameterTable->setItem(row, ParameterUnitColumn, unitItem);
            }
        }
    }

    m_isRefreshingEditor = false;
}

void AxisPerformancePage::applyTestTemplate(AxisPerformanceTestItem* testItem, const QString& testKey)
{
    if (!testItem) {
        return;
    }

    const QVector<AxisPerformanceTestItem> templates = defaultTestTemplates();
    for (const AxisPerformanceTestItem& templateItem : templates) {
        if (templateItem.key == testKey) {
            *testItem = templateItem;
            return;
        }
    }
}

QString AxisPerformancePage::displayNameForTestKey(const QString& testKey) const
{
    const QVector<AxisPerformanceTestItem> templates = defaultTestTemplates();
    for (const AxisPerformanceTestItem& templateItem : templates) {
        if (templateItem.key == testKey) {
            const QString fallback = templateItem.name.trimmed().isEmpty() ? testKey : templateItem.name.trimmed();
            return localizedTestName(m_languageCode, testKey, fallback);
        }
    }
    return localizedTestName(m_languageCode, testKey, testKey);
}

QString AxisPerformancePage::parameterDisplayName(const AxisPerformanceParameter& parameter) const
{
    const QString fallback = parameter.label.trimmed().isEmpty() ? parameter.key : parameter.label.trimmed();
    return localizedParameterName(m_languageCode, parameter.key, fallback);
}

QString AxisPerformancePage::parameterSetDisplayName(const AxisPerformanceParameterSet& parameterSet, int index) const
{
    return localizedParameterSetName(m_languageCode, parameterSet.name, index);
}

bool AxisPerformancePage::axisHasTestKey(const AxisPerformanceAxis& axis, const QString& testKey, int ignoreIndex) const
{
    for (int index = 0; index < axis.testItems.size(); ++index) {
        if (index == ignoreIndex) {
            continue;
        }
        if (axis.testItems.at(index).key == testKey) {
            return true;
        }
    }
    return false;
}

bool AxisPerformancePage::supportsParameterSets(const AxisPerformanceTestItem& testItem) const
{
    return testItem.key == QStringLiteral("settling_time") || !testItem.parameterSets.isEmpty();
}

QVector<AxisPerformanceTestItem> AxisPerformancePage::defaultTestTemplates() const
{
    QVector<AxisPerformanceTestItem> templates;

    auto appendTemplates = [&templates](const QVector<AxisPerformanceProfile>& profiles) {
        for (const AxisPerformanceProfile& profile : profiles) {
            for (const AxisPerformanceAxis& axis : profile.axes) {
                for (const AxisPerformanceTestItem& testItem : axis.testItems) {
                    bool exists = false;
                    for (const AxisPerformanceTestItem& existing : templates) {
                        if (existing.key == testItem.key) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        templates.push_back(testItem);
                    }
                }
            }
        }
    };

    appendTemplates(m_profiles);
    if (templates.isEmpty()) {
        appendTemplates(m_editorProfiles);
    }
    return templates;
}

QVector<AxisPerformanceParameter>* AxisPerformancePage::currentEditorParameters()
{
    AxisPerformanceTestItem* testItem = currentEditorTestItem();
    if (!testItem) {
        return nullptr;
    }

    if (supportsParameterSets(*testItem)) {
        const int index = currentEditorParameterSetIndex();
        if (index < 0 || index >= testItem->parameterSets.size()) {
            return nullptr;
        }
        return &testItem->parameterSets[index].parameters;
    }

    return &testItem->parameters;
}

const QVector<AxisPerformanceParameter>* AxisPerformancePage::currentEditorParameters() const
{
    const AxisPerformanceTestItem* testItem = currentEditorTestItem();
    if (!testItem) {
        return nullptr;
    }

    if (supportsParameterSets(*testItem)) {
        const int index = currentEditorParameterSetIndex();
        if (index < 0 || index >= testItem->parameterSets.size()) {
            return nullptr;
        }
        return &testItem->parameterSets.at(index).parameters;
    }

    return &testItem->parameters;
}

int AxisPerformancePage::currentEditorParameterSetIndex() const
{
    return m_parameterSetCombo ? m_parameterSetCombo->currentIndex() : -1;
}

bool AxisPerformancePage::validateProfiles(const QVector<AxisPerformanceProfile>& profiles, QString* errorMessage) const
{
    if (profiles.isEmpty()) {
        if (errorMessage) {
            *errorMessage = trText(m_languageCode,
                                   QStringLiteral("At least one profile is required"),
                                   QString::fromUtf8(u8"至少需要一个配置。"));
        }
        return false;
    }

    for (int profileIndex = 0; profileIndex < profiles.size(); ++profileIndex) {
        const AxisPerformanceProfile& profile = profiles.at(profileIndex);
        if (profile.profileName.trimmed().isEmpty()) {
            if (errorMessage) {
                *errorMessage = trText(m_languageCode,
                                       QStringLiteral("Profile %1 is missing a name").arg(profileIndex + 1),
                                       QString::fromUtf8(u8"配置 %1 缺少名称。").arg(profileIndex + 1));
            }
            return false;
        }

        for (int axisIndex = 0; axisIndex < profile.axes.size(); ++axisIndex) {
            const AxisPerformanceAxis& axis = profile.axes.at(axisIndex);
            if (axis.axisName.trimmed().isEmpty()) {
                if (errorMessage) {
                    *errorMessage = trText(m_languageCode,
                                           QStringLiteral("Axis %1 in profile %2 is missing a name").arg(axisIndex + 1).arg(profile.profileName),
                                           QString::fromUtf8(u8"\u914D\u7F6E %2 \u4E2D\u7684\u8F74 %1 \u7F3A\u5C11\u540D\u79F0").arg(axisIndex + 1).arg(profile.profileName));
                }
                return false;
            }

            for (int testIndex = 0; testIndex < axis.testItems.size(); ++testIndex) {
                const AxisPerformanceTestItem& testItem = axis.testItems.at(testIndex);
                if (testItem.key.trimmed().isEmpty() || testItem.name.trimmed().isEmpty()) {
                    if (errorMessage) {
                        *errorMessage = trText(m_languageCode,
                                               QStringLiteral("Test item %1 on axis %2 is invalid").arg(testIndex + 1).arg(axis.axisName),
                                               QString::fromUtf8(u8"\u8F74 %2 \u4E0A\u7684\u6D4B\u8BD5\u9879 %1 \u65E0\u6548").arg(testIndex + 1).arg(axis.axisName));
                    }
                    return false;
                }

                if (supportsParameterSets(testItem)) {
                    if (testItem.parameterSets.isEmpty()) {
                        if (errorMessage) {
                            *errorMessage = QStringLiteral("parameter set missing");
                        }
                        return false;
                    }

                    for (const AxisPerformanceParameterSet& parameterSet : testItem.parameterSets) {
                        for (const AxisPerformanceParameter& parameter : parameterSet.parameters) {
                            if (parameter.key.trimmed().isEmpty() || parameter.label.trimmed().isEmpty()) {
                                if (errorMessage) {
                                    *errorMessage = QStringLiteral("parameter invalid");
                                }
                                return false;
                            }
                        }
                    }
                    continue;
                }

                for (int parameterIndex = 0; parameterIndex < testItem.parameters.size(); ++parameterIndex) {
                    const AxisPerformanceParameter& parameter = testItem.parameters.at(parameterIndex);
                    if (parameter.key.trimmed().isEmpty() || parameter.label.trimmed().isEmpty()) {
                        if (errorMessage) {
                            *errorMessage = trText(m_languageCode,
                                                   QStringLiteral("Parameter %1 in test %2 is invalid").arg(parameterIndex + 1).arg(testItem.name),
                                                   QString::fromUtf8(u8"测试 %2 中的参数 %1 无效。").arg(parameterIndex + 1).arg(testItem.name));
                        }
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

AxisPerformanceProfile* AxisPerformancePage::currentEditorProfile()
{
    const int index = currentEditorProfileIndex();
    if (index < 0 || index >= m_editorProfiles.size()) {
        return nullptr;
    }
    return &m_editorProfiles[index];
}

const AxisPerformanceProfile* AxisPerformancePage::currentEditorProfile() const
{
    const int index = currentEditorProfileIndex();
    if (index < 0 || index >= m_editorProfiles.size()) {
        return nullptr;
    }
    return &m_editorProfiles.at(index);
}

AxisPerformanceAxis* AxisPerformancePage::currentEditorAxis()
{
    AxisPerformanceProfile* profile = currentEditorProfile();
    const int index = currentEditorAxisIndex();
    if (!profile || index < 0 || index >= profile->axes.size()) {
        return nullptr;
    }
    return &profile->axes[index];
}

const AxisPerformanceAxis* AxisPerformancePage::currentEditorAxis() const
{
    const AxisPerformanceProfile* profile = currentEditorProfile();
    const int index = currentEditorAxisIndex();
    if (!profile || index < 0 || index >= profile->axes.size()) {
        return nullptr;
    }
    return &profile->axes.at(index);
}

AxisPerformanceTestItem* AxisPerformancePage::currentEditorTestItem()
{
    AxisPerformanceAxis* axis = currentEditorAxis();
    const int index = currentEditorTestIndex();
    if (!axis || index < 0 || index >= axis->testItems.size()) {
        return nullptr;
    }
    return &axis->testItems[index];
}

const AxisPerformanceTestItem* AxisPerformancePage::currentEditorTestItem() const
{
    const AxisPerformanceAxis* axis = currentEditorAxis();
    const int index = currentEditorTestIndex();
    if (!axis || index < 0 || index >= axis->testItems.size()) {
        return nullptr;
    }
    return &axis->testItems.at(index);
}

int AxisPerformancePage::currentEditorProfileIndex() const
{
    return m_editorProfileCombo ? m_editorProfileCombo->currentIndex() : -1;
}

int AxisPerformancePage::currentEditorAxisIndex() const
{
    return m_axisListWidget ? m_axisListWidget->currentRow() : -1;
}

int AxisPerformancePage::currentEditorTestIndex() const
{
    return m_testTable ? m_testTable->currentRow() : -1;
}

QString AxisPerformancePage::textForKey(const QString& key) const
{
    if (key == QStringLiteral("settings")) return trText(m_languageCode, QStringLiteral("Settings"), QString::fromUtf8(u8"\u8BBE\u7F6E"));
    if (key == QStringLiteral("ready")) return trText(m_languageCode, QStringLiteral("Ready"), QString::fromUtf8(u8"\u5C31\u7EEA"));
    if (key == QStringLiteral("runtime_log")) return QString();
    if (key == QStringLiteral("editor_title")) return trText(m_languageCode, QStringLiteral("Axis Performance Settings"), QString::fromUtf8(u8"\u8F74\u6027\u80FD\u8BBE\u7F6E"));
    if (key == QStringLiteral("browse")) return trText(m_languageCode, QStringLiteral("Browse"), QString::fromUtf8(u8"\u6D4F\u89C8"));
    if (key == QStringLiteral("save")) return trText(m_languageCode, QStringLiteral("Save"), QString::fromUtf8(u8"\u4FDD\u5B58"));
    if (key == QStringLiteral("profile_select")) return trText(m_languageCode, QStringLiteral("Profile"), QString::fromUtf8(u8"\u914D\u7F6E"));
    if (key == QStringLiteral("profile_name")) return trText(m_languageCode, QStringLiteral("Profile Name"), QString::fromUtf8(u8"\u914D\u7F6E\u540D\u79F0"));
    if (key == QStringLiteral("profile_desc")) return trText(m_languageCode, QStringLiteral("Description"), QString::fromUtf8(u8"\u8BF4\u660E"));
    if (key == QStringLiteral("axes_tab")) return trText(m_languageCode, QStringLiteral("Axes"), QString::fromUtf8(u8"\u8F74\u5217\u8868"));
    if (key == QStringLiteral("tests_tab")) return trText(m_languageCode, QStringLiteral("Test Items"), QString::fromUtf8(u8"\u6D4B\u8BD5\u9879"));
    if (key == QStringLiteral("parameters_tab")) return trText(m_languageCode, QStringLiteral("Parameters"), QString::fromUtf8(u8"\u53C2\u6570"));
    if (key == QStringLiteral("axis_name")) return trText(m_languageCode, QStringLiteral("Axis Name"), QString::fromUtf8(u8"\u8F74\u540D\u79F0"));
    if (key == QStringLiteral("axis_number")) return trText(m_languageCode, QStringLiteral("Axis No."), QString::fromUtf8(u8"\u8F74\u53F7"));
    if (key == QStringLiteral("unit")) return trText(m_languageCode, QStringLiteral("Unit"), QString::fromUtf8(u8"\u5355\u4F4D"));
    if (key == QStringLiteral("test_name")) return trText(m_languageCode, QStringLiteral("Test Name"), QString::fromUtf8(u8"\u6D4B\u8BD5\u540D\u79F0"));
    if (key == QStringLiteral("test_desc")) return trText(m_languageCode, QStringLiteral("Test Description"), QString::fromUtf8(u8"\u6D4B\u8BD5\u8BF4\u660E"));
    if (key == QStringLiteral("param_name")) return trText(m_languageCode, QStringLiteral("Parameter"), QString::fromUtf8(u8"\u53C2\u6570\u540D"));
    if (key == QStringLiteral("param_value")) return trText(m_languageCode, QStringLiteral("Param Value"), QString::fromUtf8(u8"\u53C2\u6570\u503C"));
    if (key == QStringLiteral("add_profile")) return trText(m_languageCode, QStringLiteral("Add Profile"), QString::fromUtf8(u8"\u65B0\u589E\u914D\u7F6E"));
    if (key == QStringLiteral("remove_profile")) return trText(m_languageCode, QStringLiteral("Delete Profile"), QString::fromUtf8(u8"\u5220\u9664\u914D\u7F6E"));
    if (key == QStringLiteral("add_axis")) return trText(m_languageCode, QStringLiteral("Add Axis"), QString::fromUtf8(u8"\u65B0\u589E\u8F74"));
    if (key == QStringLiteral("remove_axis")) return trText(m_languageCode, QStringLiteral("Delete Axis"), QString::fromUtf8(u8"\u5220\u9664\u8F74"));
    if (key == QStringLiteral("add_test")) return trText(m_languageCode, QStringLiteral("Add Test"), QString::fromUtf8(u8"\u65B0\u589E\u6D4B\u8BD5"));
    if (key == QStringLiteral("remove_test")) return trText(m_languageCode, QStringLiteral("Delete Test"), QString::fromUtf8(u8"\u5220\u9664\u6D4B\u8BD5"));
    if (key == QStringLiteral("unnamed_profile")) return trText(m_languageCode, QStringLiteral("Unnamed Profile"), QString::fromUtf8(u8"\u672A\u547D\u540D\u914D\u7F6E"));
    if (key == QStringLiteral("unnamed_axis")) return trText(m_languageCode, QStringLiteral("Unnamed Axis"), QString::fromUtf8(u8"\u672A\u547D\u540D\u8F74"));
    if (key == QStringLiteral("add_param_set")) return trText(m_languageCode, QStringLiteral("Add Set"), QString::fromUtf8(u8"\u65B0\u589E\u53C2\u6570\u7EC4"));
    if (key == QStringLiteral("remove_param_set")) return trText(m_languageCode, QStringLiteral("Delete Set"), QString::fromUtf8(u8"\u5220\u9664\u53C2\u6570\u7EC4"));
    return key;
}

QString AxisPerformancePage::configFilePath() const
{
    if (m_editorPathLabel) {
        const QString overridePath = m_editorPathLabel->property("configPath").toString();
        if (!overridePath.isEmpty()) {
            return overridePath;
        }
    }
    return QStringLiteral("Config/axis_performance_test_simulator.json");
}

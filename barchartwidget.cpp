#include "barchartwidget.h"
#include "salesstore.h"

#include <QFontMetrics>
#include <QPainter>
#include <algorithm>
#include <cmath>

BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize BarChartWidget::sizeHint() const
{
    return QSize(240, 170);
}

void BarChartWidget::setBars(const QVector<ChartBar> &bars)
{
    m_bars = bars;
    QStringList details;
    for (const ChartBar &bar : m_bars) {
        QString text = QStringLiteral("%1 : %2 %3").arg(bar.label, formatQuantity(bar.value), bar.unit);
        if (bar.threshold >= 0.0)
            text += QStringLiteral(" (seuil : %1 %2)").arg(formatQuantity(bar.threshold), bar.unit);
        details.append(text);
    }
    setToolTip(details.join(QLatin1Char('\n')));
    update();
}

void BarChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);

    QFont smallFont = font();
    smallFont.setPixelSize(9);
    painter.setFont(smallFont);
    if (m_bars.isEmpty()) {
        painter.setPen(QColor(QStringLiteral("#8190a3")));
        painter.drawText(rect().adjusted(8, 8, -8, -8), Qt::AlignCenter | Qt::TextWordWrap,
                         QStringLiteral("Aucune livraison\npour cet article"));
        return;
    }
    double maximum = 1.0;
    for (const ChartBar &bar : m_bars)
        maximum = std::max(maximum, std::max(bar.value, bar.threshold));
    const double step = std::pow(10.0, std::floor(std::log10(maximum))) / 10.0;
    maximum = std::ceil((maximum * 1.17) / step) * step;

    const QString topLabel = formatQuantity(maximum);
    const int left = std::max(27, painter.fontMetrics().horizontalAdvance(topLabel) + 7);
    const QRectF plot(left, 18, std::max(1, width() - left - 5), std::max(1, height() - 48));

    for (int grid = 0; grid <= 2; ++grid) {
        const double value = maximum * static_cast<double>(grid) / 2.0;
        const double y = plot.bottom() - plot.height() * static_cast<double>(grid) / 2.0;
        painter.setPen(QPen(QColor(QStringLiteral("#e6eaf0")), 1));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor(QStringLiteral("#7b8796")));
        painter.drawText(QRectF(0, y - 7, left - 5, 14), Qt::AlignRight | Qt::AlignVCenter,
                         formatQuantity(value));
    }
    const double slot = plot.width() / static_cast<double>(m_bars.size());
    const double barWidth = std::min(36.0, slot * 0.52);
    for (int index = 0; index < static_cast<int>(m_bars.size()); ++index) {
        const ChartBar &bar = m_bars.at(index);
        const double center = plot.left() + slot * (static_cast<double>(index) + 0.5);
        const double barHeight = plot.height() * std::max(0.0, bar.value) / maximum;
        const QRectF barRect(center - barWidth / 2.0, plot.bottom() - barHeight, barWidth, barHeight);
        painter.setPen(Qt::NoPen);
        painter.setBrush(bar.color);
        painter.drawRoundedRect(barRect, 2, 2);
        // Cover the rounded bottom to keep bars anchored to the baseline.
        if (barHeight > 2.0)
            painter.fillRect(QRectF(barRect.left(), plot.bottom() - 2, barWidth, 2), bar.color);

        QFont numberFont = smallFont;
        numberFont.setBold(true);
        painter.setFont(numberFont);
        painter.setPen(QColor(QStringLiteral("#263b52")));
        painter.drawText(QRectF(center - slot / 2.0, barRect.top() - 16, slot, 14),
                         Qt::AlignHCenter | Qt::AlignVCenter, formatQuantity(bar.value));

        if (bar.threshold >= 0.0) {
            QPen thresholdPen(QColor(QStringLiteral("#dba449")), 1.4, Qt::DashLine);
            painter.setPen(thresholdPen);
            const double y = plot.bottom() - plot.height() * bar.threshold / maximum;
            painter.drawLine(QPointF(center - slot * 0.32, y), QPointF(center + slot * 0.32, y));
        }
        painter.setFont(smallFont);
        painter.setPen(QColor(QStringLiteral("#7b8796")));
        const QString label = painter.fontMetrics().elidedText(bar.label, Qt::ElideRight,
                                                               std::max(1, static_cast<int>(slot) - 3));
        painter.drawText(QRectF(center - slot / 2.0, plot.bottom() + 5, slot, 20),
                         Qt::AlignHCenter | Qt::AlignTop, label);
    }
}

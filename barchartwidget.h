#pragma once

#include <QColor>
#include <QVector>
#include <QWidget>

struct ChartBar {
    QString label;
    double value = 0.0;
    QColor color;
    double threshold = -1.0; // Negative = no threshold line.
    QString unit;
};

// Lightweight bar charts: this uses QPainter, not the Qt Charts module.
class BarChartWidget : public QWidget {
public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    void setBars(const QVector<ChartBar> &bars);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<ChartBar> m_bars;
};

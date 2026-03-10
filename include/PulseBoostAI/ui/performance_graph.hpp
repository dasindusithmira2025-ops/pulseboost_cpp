#pragma once

#include <cstddef>
#include <deque>

#include <QColor>
#include <QString>
#include <QWidget>

namespace pulseboost {

class PerformanceGraph : public QWidget {
public:
    explicit PerformanceGraph(const QString &title, const QColor &lineColor, QWidget *parent = nullptr);
    void pushSample(double value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString title_;
    QColor lineColor_;
    std::deque<double> samples_;
    static constexpr std::size_t kMaxSamples = 60;
};

}  // namespace pulseboost

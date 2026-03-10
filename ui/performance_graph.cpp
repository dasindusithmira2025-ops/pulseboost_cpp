#include "PulseBoostAI/ui/performance_graph.hpp"

#include <algorithm>

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

namespace pulseboost {

PerformanceGraph::PerformanceGraph(const QString &title, const QColor &lineColor, QWidget *parent)
    : QWidget(parent), title_(title), lineColor_(lineColor) {
    setMinimumHeight(180);
}

void PerformanceGraph::pushSample(double value) {
    if (samples_.size() >= kMaxSamples) {
        samples_.pop_front();
    }
    samples_.push_back(std::clamp(value, 0.0, 100.0));
    update();
}

void PerformanceGraph::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#101622"));

    painter.setPen(QColor("#30415f"));
    for (int index = 1; index < 4; ++index) {
        const int y = rect().top() + (rect().height() * index / 4);
        painter.drawLine(rect().left() + 12, y, rect().right() - 12, y);
    }

    painter.setPen(Qt::white);
    painter.drawText(rect().adjusted(12, 10, -12, -10), Qt::AlignTop | Qt::AlignLeft, title_);

    if (samples_.size() < 2) {
        return;
    }

    QPainterPath path;
    const QRect graphRect = rect().adjusted(12, 36, -12, -16);
    for (std::size_t index = 0; index < samples_.size(); ++index) {
        const double xRatio = static_cast<double>(index) / static_cast<double>(samples_.size() - 1);
        const double yRatio = samples_[index] / 100.0;
        const QPointF point(graphRect.left() + xRatio * graphRect.width(),
                            graphRect.bottom() - yRatio * graphRect.height());
        if (index == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }

    painter.setPen(QPen(lineColor_, 2.5));
    painter.drawPath(path);
}

}  // namespace pulseboost

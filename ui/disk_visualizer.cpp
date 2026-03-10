#include "PulseBoostAI/ui/disk_visualizer.hpp"

#include <numeric>

#include <QColor>
#include <QPainter>
#include <QPaintEvent>

namespace pulseboost {

DiskVisualizer::DiskVisualizer(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(240);
}

void DiskVisualizer::setCategories(std::vector<StorageCategory> categories) {
    categories_ = std::move(categories);
    update();
}

void DiskVisualizer::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor("#161d2d"));

    painter.setPen(Qt::white);
    painter.drawText(rect().adjusted(12, 10, -12, -10), Qt::AlignTop | Qt::AlignLeft, "Storage Map");

    const std::uint64_t total = std::accumulate(categories_.begin(), categories_.end(), std::uint64_t {0},
                                                [](std::uint64_t running, const StorageCategory &category) {
                                                    return running + category.bytes;
                                                });
    if (total == 0 || categories_.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, "No storage data yet");
        return;
    }

    const QList<QColor> colors = {QColor("#c96a2b"), QColor("#2e86de"), QColor("#e74c3c"), QColor("#2ecc71"),
                                  QColor("#9b59b6"), QColor("#f1c40f")};
    QRectF barRect(rect().left() + 12.0, rect().top() + 40.0, rect().width() - 24.0, 30.0);
    double xOffset = barRect.left();
    for (std::size_t index = 0; index < categories_.size(); ++index) {
        const double width = (static_cast<double>(categories_[index].bytes) / static_cast<double>(total)) * barRect.width();
        painter.fillRect(QRectF(xOffset, barRect.top(), width, barRect.height()), colors[static_cast<int>(index % colors.size())]);
        xOffset += width;
    }

    int legendY = 90;
    for (std::size_t index = 0; index < categories_.size(); ++index) {
        painter.fillRect(QRect(16, legendY, 12, 12), colors[static_cast<int>(index % colors.size())]);
        const auto &category = categories_[index];
        painter.drawText(QRect(36, legendY - 2, width() - 48, 20), Qt::AlignLeft | Qt::AlignVCenter,
                         QString::fromStdString(category.name + " - " + std::to_string(category.bytes / (1024ULL * 1024ULL * 1024ULL)) +
                                                " GB"));
        legendY += 24;
    }
}

}  // namespace pulseboost

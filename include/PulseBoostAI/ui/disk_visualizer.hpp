#pragma once

#include <vector>

#include <QWidget>

#include "PulseBoostAI/common/models.hpp"

namespace pulseboost {

class DiskVisualizer : public QWidget {
public:
    explicit DiskVisualizer(QWidget *parent = nullptr);
    void setCategories(std::vector<StorageCategory> categories);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<StorageCategory> categories_;
};

}  // namespace pulseboost

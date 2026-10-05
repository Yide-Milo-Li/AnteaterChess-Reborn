#pragma once
#include "anteater/rules.hpp"
#include <QAbstractListModel>
#include <array>

namespace ac {
class BoardModel : public QAbstractListModel {
    Q_OBJECT
  public:
    enum Role { Row = Qt::UserRole + 1, Column, Coordinate, Asset, Selected, Legal, HintFrom, HintTo };
    explicit BoardModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void update(const Position &position);
    void highlights(Square selected, const std::array<bool, Rows * Columns> &legal);
    void hint(const Move *move);

  private:
    Position position_{};
    Square selected_{-1, -1}, hintFrom_{-1, -1}, hintTo_{-1, -1};
    std::array<bool, Rows * Columns> legal_{};
};
} // namespace ac

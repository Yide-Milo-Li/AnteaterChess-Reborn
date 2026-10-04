#pragma once
#include "anteater/rules.h"
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
    void update(const AcPosition &position);
    void highlights(AcSquare selected, const std::array<bool, AC_ROWS * AC_COLS> &legal);
    void hint(const AcMove *move);
private:
    AcPosition position_{};
    AcSquare selected_{-1,-1}, hintFrom_{-1,-1}, hintTo_{-1,-1};
    std::array<bool, AC_ROWS * AC_COLS> legal_{};
};
}

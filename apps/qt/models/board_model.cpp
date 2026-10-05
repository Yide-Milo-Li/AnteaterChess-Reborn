#include "board_model.hpp"
#include "runtime/runtime.hpp"

using namespace ac;
namespace ac {
BoardModel::BoardModel(QObject *parent) : QAbstractListModel(parent) {
    position_init(&position_);
}
int BoardModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : Rows * Columns;
}
QVariant BoardModel::data(const QModelIndex &i, int role) const {
    if (!i.isValid() || i.row() < 0 || i.row() >= rowCount())
        return {};
    Square s{i.row() / Columns, i.row() % Columns};
    switch (role) {
    case Row:
        return s.row;
    case Column:
        return s.col;
    case Coordinate:
        return QString(QChar('A' + s.col)) + QString::number(8 - s.row);
    case Asset:
        return pieceAsset(get_piece(&position_.board, s));
    case Selected:
        return bool(position_equal(s, selected_));
    case Legal:
        return legal_[size_t(i.row())];
    case HintFrom:
        return bool(position_equal(s, hintFrom_));
    case HintTo:
        return bool(position_equal(s, hintTo_));
    default:
        return {};
    }
}
QHash<int, QByteArray> BoardModel::roleNames() const {
    return {
        {Row,        "boardRow"   },
        {Column,     "boardColumn"},
        {Coordinate, "coordinate" },
        {Asset,      "asset"      },
        {Selected,   "selected"   },
        {Legal,      "legal"      },
        {HintFrom,   "hintFrom"   },
        {HintTo,     "hintTo"     }
    };
}
void BoardModel::update(const Position &p) {
    if (position_.hash == p.hash)
        return;
    position_ = p;
    emit dataChanged(index(0), index(rowCount() - 1), {Asset});
}
void BoardModel::highlights(Square selected, const std::array<bool, Rows * Columns> &legal) {
    selected_ = selected;
    legal_ = legal;
    emit dataChanged(index(0), index(rowCount() - 1), {Selected, Legal});
}
void BoardModel::hint(const Move *move) {
    hintFrom_ = move ? move->from : Square{-1, -1};
    hintTo_ = move ? move->to : Square{-1, -1};
    emit dataChanged(index(0), index(rowCount() - 1), {HintFrom, HintTo});
}
} // namespace ac

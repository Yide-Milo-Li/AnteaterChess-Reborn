#include "board_model.h"
#include "runtime/runtime.h"
namespace ac {
BoardModel::BoardModel(QObject *parent) : QAbstractListModel(parent) { ac_position_init(&position_); }
int BoardModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : AC_ROWS * AC_COLS; }
QVariant BoardModel::data(const QModelIndex &i, int role) const {
    if (!i.isValid() || i.row() < 0 || i.row() >= rowCount()) return {};
    AcSquare s{i.row() / AC_COLS, i.row() % AC_COLS};
    switch (role) {
    case Row: return s.row;
    case Column: return s.col;
    case Coordinate: return QString(QChar('A' + s.col)) + QString::number(8-s.row);
    case Asset: return pieceAsset(ac_get_piece(&position_.board, s));
    case Selected: return bool(ac_position_equal(s, selected_));
    case Legal: return legal_[size_t(i.row())];
    case HintFrom: return bool(ac_position_equal(s, hintFrom_));
    case HintTo: return bool(ac_position_equal(s, hintTo_));
    default: return {};
    }
}
QHash<int, QByteArray> BoardModel::roleNames() const {
    return {{Row,"boardRow"},{Column,"boardColumn"},{Coordinate,"coordinate"},{Asset,"asset"},
        {Selected,"selected"},{Legal,"legal"},{HintFrom,"hintFrom"},{HintTo,"hintTo"}};
}
void BoardModel::update(const AcPosition &p) {
    if (position_.hash == p.hash) return;
    position_ = p;
    emit dataChanged(index(0), index(rowCount()-1), {Asset});
}
void BoardModel::highlights(AcSquare selected, const std::array<bool, AC_ROWS * AC_COLS> &legal) {
    selected_ = selected; legal_ = legal;
    emit dataChanged(index(0), index(rowCount()-1), {Selected,Legal});
}
void BoardModel::hint(const AcMove *move) {
    hintFrom_ = move ? move->from : AcSquare{-1,-1};
    hintTo_ = move ? move->to : AcSquare{-1,-1};
    emit dataChanged(index(0), index(rowCount()-1), {HintFrom,HintTo});
}
}

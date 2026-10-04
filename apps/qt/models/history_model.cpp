#include "history_model.h"
namespace ac {
int HistoryModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : int(moves_.size()); }
QVariant HistoryModel::data(const QModelIndex &i, int role) const {
    if (!i.isValid() || i.row() < 0 || i.row() >= rowCount() || role != Qt::UserRole + 1) return {};
    const AcMove &m = moves_[size_t(i.row())];
    QString text = QString("%1. %2  %3%4 → %5%6").arg(i.row()+1)
        .arg(m.movedPiece.color == AC_WHITE ? "White" : "Black")
        .arg(QChar('A'+m.from.col)).arg(8-m.from.row).arg(QChar('A'+m.to.col)).arg(8-m.to.row);
    if (m.captureCount) text += QString("  ×%1").arg(m.captureCount);
    if (ac_is_promotion_special_move(m.specialType)) text += "  promotion";
    return text;
}
void HistoryModel::update(const AcSnapshot &s) {
    // Append incrementally; ticking clocks must not reset delegates or scroll.
    if (gameId_ != s.gameId || s.historyCount < rowCount()) {
        beginResetModel();
        moves_.clear();
        if (s.historyCount) moves_.assign(s.history, s.history+s.historyCount);
        gameId_ = s.gameId;
        endResetModel();
    } else if (s.historyCount > rowCount()) {
        int old = rowCount();
        beginInsertRows({}, old, s.historyCount-1);
        moves_.insert(moves_.end(), s.history+old, s.history+s.historyCount);
        endInsertRows();
    }
}
}

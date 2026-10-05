#include "history_model.h"

using namespace ac;
namespace ac {
int HistoryModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : int(moves_.size());
}
QVariant HistoryModel::data(const QModelIndex &i, int role) const {
    if (!i.isValid() || i.row() < 0 || i.row() >= rowCount() || role != Qt::UserRole + 1)
        return {};
    const Move &m = moves_[size_t(i.row())];
    QString text = QString("%1. %2  %3%4 → %5%6")
                       .arg(i.row() + 1)
                       .arg(m.movedPiece.color == Color::White ? "White" : "Black")
                       .arg(QChar('A' + m.from.col))
                       .arg(8 - m.from.row)
                       .arg(QChar('A' + m.to.col))
                       .arg(8 - m.to.row);
    if (m.captureCount)
        text += QString("  ×%1").arg(m.captureCount);
    if (is_promotion_special_move(m.specialType))
        text += "  promotion";
    return text;
}
void HistoryModel::update(const SessionSnapshot &s) {
    // Append incrementally; ticking clocks must not reset delegates or scroll.
    if (gameId_ != s.gameId || s.historyCount < rowCount()) {
        std::vector<Move> prepared;
        if (!s.history.empty()) prepared.assign(s.history.begin(), s.history.end());
        beginResetModel();
        moves_.swap(prepared);
        gameId_ = s.gameId;
        endResetModel();
    } else if (s.historyCount > rowCount()) {
        int old = rowCount();
        moves_.reserve(s.history.size());
        beginInsertRows({}, old, s.historyCount - 1);
        moves_.insert(moves_.end(), s.history.begin() + old, s.history.end());
        endInsertRows();
    }
}
} // namespace ac

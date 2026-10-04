#pragma once
#include "anteater/session.h"
#include <QAbstractListModel>
#include <vector>
namespace ac {
class HistoryModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit HistoryModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override { return {{Qt::UserRole + 1,"moveText"}}; }
    void update(const AcSnapshot &snapshot);
private:
    std::vector<AcMove> moves_;
    uint64_t gameId_ = 0;
};
}

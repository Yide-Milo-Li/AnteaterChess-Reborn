#pragma once
#include "anteater/session.hpp"
#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <vector>

namespace ac {
class HistoryModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ApplicationController")
  public:
    explicit HistoryModel(QObject *parent = nullptr) : QAbstractListModel(parent) {
    }
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override {
        return {
            {Qt::UserRole + 1, "moveText"}
        };
    }
    void update(const SessionSnapshot &snapshot);

  private:
    std::vector<Move> moves_;
    uint64_t gameId_ = 0;
};
} // namespace ac

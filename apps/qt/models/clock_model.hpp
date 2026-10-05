#pragma once
#include "anteater/session.hpp"
#include <QObject>
#include <QtQml/qqmlregistration.h>
namespace ac {
// Only this model is notified by ordinary 100 ms clock polling.
class ClockModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ApplicationController")
    Q_PROPERTY(QString elapsed READ elapsed NOTIFY elapsedChanged)
    Q_PROPERTY(QString white READ white NOTIFY whiteChanged)
    Q_PROPERTY(QString black READ black NOTIFY blackChanged)
  public:
    const QString &elapsed() const {
        return elapsed_;
    }
    const QString &white() const {
        return white_;
    }
    const QString &black() const {
        return black_;
    }
    void update(const SessionState &state);
  signals:
    void elapsedChanged();
    void whiteChanged();
    void blackChanged();

  private:
    QString elapsed_, white_, black_;
};
} // namespace ac

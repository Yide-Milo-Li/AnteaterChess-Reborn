#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>
namespace ac {
class InputModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ApplicationController")
    Q_PROPERTY(QString fromText READ fromText NOTIFY fieldsChanged)
    Q_PROPERTY(QString toText READ toText NOTIFY fieldsChanged)
    Q_PROPERTY(bool fromValid READ fromValid NOTIFY validityChanged)
    Q_PROPERTY(bool toValid READ toValid NOTIFY validityChanged)
  public:
    const QString &fromText() const {
        return from_;
    }
    const QString &toText() const {
        return to_;
    }
    bool fromValid() const {
        return fromValid_;
    }
    bool toValid() const {
        return toValid_;
    }
    void setFields(const QString &from, const QString &to) {
        if (from_ == from && to_ == to)
            return;
        from_ = from;
        to_ = to;
        emit fieldsChanged();
    }
    void setValidity(bool from, bool to) {
        if (fromValid_ == from && toValid_ == to)
            return;
        fromValid_ = from;
        toValid_ = to;
        emit validityChanged();
    }
  signals:
    void fieldsChanged();
    void validityChanged();

  private:
    QString from_, to_;
    bool fromValid_ = false, toValid_ = false;
};
} // namespace ac

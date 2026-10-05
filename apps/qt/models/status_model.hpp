#pragma once
#include "anteater/session.hpp"
#include <QObject>
#include <QtQml/qqmlregistration.h>
namespace ac {
class StatusModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by ApplicationController")
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool error READ error NOTIFY messageChanged)
    Q_PROPERTY(bool humanTurn READ humanTurn NOTIFY availabilityChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY availabilityChanged)
    Q_PROPERTY(bool canHint READ canHint NOTIFY availabilityChanged)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY historyCountChanged)
    Q_PROPERTY(QString turnText READ turnText NOTIFY turnChanged)
    Q_PROPERTY(QString modeText READ modeText NOTIFY configurationChanged)
    Q_PROPERTY(QString aiSummary READ aiSummary NOTIFY configurationChanged)
    Q_PROPERTY(QString resultText READ resultText NOTIFY resultChanged)
  public:
    const QString &message() const {
        return message_;
    }
    bool error() const {
        return error_;
    }
    bool humanTurn() const {
        return humanTurn_;
    }
    bool canUndo() const {
        return canUndo_;
    }
    bool canHint() const {
        return canHint_;
    }
    int historyCount() const {
        return count_;
    }
    const QString &turnText() const {
        return turn_;
    }
    const QString &modeText() const {
        return mode_;
    }
    const QString &aiSummary() const {
        return ai_;
    }
    const QString &resultText() const {
        return result_;
    }
    void setMessage(const QString &text, bool error) {
        if (message_ == text && error_ == error)
            return;
        message_ = text;
        error_ = error;
        emit messageChanged();
    }
    void update(const SessionState &state, bool gameplay, bool busy, bool closing, bool promotion);
  signals:
    void messageChanged();
    void availabilityChanged();
    void historyCountChanged();
    void turnChanged();
    void configurationChanged();
    void resultChanged();

  private:
    QString message_ = "Ready", turn_, mode_, ai_, result_;
    bool error_ = false, humanTurn_ = false, canUndo_ = false, canHint_ = false;
    int count_ = 0;
};
} // namespace ac

#pragma once
#include "anteater/session.hpp"
#include "async/search_jobs.h"
#include "models/board_model.h"
#include "models/history_model.h"
#include "runtime/runtime.h"
#include <QTimer>

namespace ac {
// Commands mutate Session; getters read the last published projection only.
class SessionAdapter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *boardModel READ boardModel CONSTANT)
    Q_PROPERTY(QObject *historyModel READ historyModel CONSTANT)
    Q_PROPERTY(int page READ page NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool humanTurn READ humanTurn NOTIFY stateChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
    Q_PROPERTY(bool canHint READ canHint NOTIFY stateChanged)
    Q_PROPERTY(int historyCount READ historyCount NOTIFY stateChanged)
    Q_PROPERTY(QString turnText READ turnText NOTIFY stateChanged)
    Q_PROPERTY(QString clockText READ clockText NOTIFY stateChanged)
    Q_PROPERTY(QString whiteTimer READ whiteTimer NOTIFY stateChanged)
    Q_PROPERTY(QString blackTimer READ blackTimer NOTIFY stateChanged)
    Q_PROPERTY(QString modeText READ modeText NOTIFY stateChanged)
    Q_PROPERTY(QString aiSummary READ aiSummary NOTIFY stateChanged)
    Q_PROPERTY(QString resultText READ resultText NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bool statusError READ statusError NOTIFY stateChanged)
    Q_PROPERTY(QString fromText READ fromText NOTIFY moveFieldsChanged)
    Q_PROPERTY(QString toText READ toText NOTIFY moveFieldsChanged)
    Q_PROPERTY(bool fromValid READ fromValid NOTIFY moveFieldsChanged)
    Q_PROPERTY(bool toValid READ toValid NOTIFY moveFieldsChanged)
  public:
    enum Page { MainMenu, ModeMenu, Setup, Gameplay, EndGame };
    Q_ENUM(Page)
    explicit SessionAdapter(const SessionOptions *options = nullptr, QObject *parent = nullptr);
    ~SessionAdapter() override;
    bool valid() const {
        return session_ != nullptr;
    }
    QObject *boardModel() {
        return &board_;
    }
    QObject *historyModel() {
        return &history_;
    }
    int page() const {
        return page_;
    }
    bool busy() const {
        return jobs_.busy();
    }
    bool humanTurn() const;
    bool canUndo() const;
    bool canHint() const;
    int historyCount() const {
        return state_.historyCount;
    }
    QString turnText() const;
    QString clockText() const;
    QString whiteTimer() const;
    QString blackTimer() const;
    QString modeText() const;
    QString aiSummary() const;
    QString resultText() const;
    QString status() const {
        return status_;
    }
    bool statusError() const {
        return error_;
    }
    QString fromText() const {
        return from_;
    }
    QString toText() const {
        return to_;
    }
    bool fromValid() const {
        return fromValid_;
    }
    bool toValid() const {
        return toValid_;
    }
    Snapshot snapshot() const;
    Status start(const GameConfig &config);
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void chooseMode(int mode);
    Q_INVOKABLE void back();
    Q_INVOKABLE bool startConfigured(int mode, int playerColor, int whiteDifficulty, int blackDifficulty,
                                     bool timerEnabled, int turnSeconds, int aiSeconds);
    Q_INVOKABLE void setMoveFields(const QString &from, const QString &to);
    Q_INVOKABLE void selectSquare(int row, int column, int button);
    Q_INVOKABLE bool submitFields(int promotion = 0);
    Q_INVOKABLE void cancelPromotion();
    Q_INVOKABLE void undo();
    Q_INVOKABLE bool hint();
    Q_INVOKABLE void finish();
    Q_INVOKABLE bool requestClose();
    void tick();
  signals:
    void stateChanged();
    void moveFieldsChanged();
    void promotionRequested();
    void promotionDismissed();
    void closeReady();

  private:
    void refresh();
    void invalidate();
    void updateHighlights();
    void searchCompleted();
    void report(Status status);
    QString timerText(Color color) const;
    Session *session_ = nullptr;
    SessionLog log_;
    BoardModel board_;
    HistoryModel history_;
    SearchJobs jobs_;
    QTimer timer_;
    Snapshot state_{}; // History/hash pointers are cleared after projection.
    Page page_ = MainMenu;
    uint64_t generation_ = 0, promotionRevision_ = 0;
    int failedCount_ = -1;
    Color failedTurn_ = Color::Empty;
    bool closing_ = false, promotionPending_ = false, error_ = false;
    bool fromValid_ = false, toValid_ = false;
    QString status_ = "Ready", from_, to_;
};
} // namespace ac

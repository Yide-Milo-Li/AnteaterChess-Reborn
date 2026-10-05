#pragma once
#include "anteater/session.hpp"
#include "async/search_jobs.hpp"
#include "models/board_model.hpp"
#include "models/history_model.hpp"
#include "models/input_model.hpp"
#include "models/clock_model.hpp"
#include "models/status_model.hpp"
#include "models/settings_draft.hpp"
#include "runtime/runtime.hpp"
#include <QTimer>
#include <optional>
namespace ac {
// Sole application owner. Models retain presentation values; commands mutate Session.
class ApplicationController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ac::BoardModel *boardModel READ boardModel CONSTANT)
    Q_PROPERTY(ac::HistoryModel *historyModel READ historyModel CONSTANT)
    Q_PROPERTY(ac::InputModel *input READ input CONSTANT)
    Q_PROPERTY(ac::ClockModel *clocks READ clocks CONSTANT)
    Q_PROPERTY(ac::StatusModel *game READ game CONSTANT)
    Q_PROPERTY(ac::SettingsDraft *settings READ settings CONSTANT)
    Q_PROPERTY(Page page READ page NOTIFY pageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  public:
    enum Page { MainMenu, ModeMenu, Setup, Gameplay, EndGame };
    Q_ENUM(Page)
    explicit ApplicationController(QObject *parent);
    explicit ApplicationController(const SessionOptions *options = nullptr, QObject *parent = nullptr,
                                   SessionLog log = SessionLog::besideExecutable(),
                                   std::pmr::memory_resource *searchResource = std::pmr::get_default_resource());
    ~ApplicationController() override;
    bool valid() const {
        return session_.has_value();
    }
    BoardModel *boardModel() {
        return &board_;
    }
    HistoryModel *historyModel() {
        return &history_;
    }
    InputModel *input() {
        return &input_;
    }
    ClockModel *clocks() {
        return &clocks_;
    }
    StatusModel *game() {
        return &game_;
    }
    SettingsDraft *settings() {
        return &settings_;
    }
    Page page() const {
        return page_;
    }
    bool busy() const {
        return jobs_.busy();
    }
    bool humanTurn() const {
        return game_.humanTurn();
    }
    bool canUndo() const {
        return game_.canUndo();
    }
    bool canHint() const {
        return game_.canHint();
    }
    int historyCount() const {
        return game_.historyCount();
    }
    QString turnText() const {
        return game_.turnText();
    }
    QString clockText() const {
        return clocks_.elapsed();
    }
    QString whiteTimer() const {
        return clocks_.white();
    }
    QString blackTimer() const {
        return clocks_.black();
    }
    QString modeText() const {
        return game_.modeText();
    }
    QString aiSummary() const {
        return game_.aiSummary();
    }
    QString resultText() const {
        return game_.resultText();
    }
    QString status() const {
        return game_.message();
    }
    bool statusError() const {
        return game_.error();
    }
    QString fromText() const {
        return input_.fromText();
    }
    QString toText() const {
        return input_.toText();
    }
    bool fromValid() const {
        return input_.fromValid();
    }
    bool toValid() const {
        return input_.toValid();
    }
    SessionState state() const;
    Result<SessionSnapshot> snapshot() const {
        return session_ ? session_->snapshot()
                        : Result<SessionSnapshot>{
                              Error{Status::Unavailable, "No Session"}
        };
    }
    Status diagnostic() const {
        return diagnostic_;
    }
    Status start(const GameConfig &config);
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void chooseMode(GameEnums::Mode mode);
    Q_INVOKABLE void back();
    Q_INVOKABLE bool startDraft();
    Q_INVOKABLE void setMoveFields(const QString &from, const QString &to);
    Q_INVOKABLE void selectSquare(int row, int column, Qt::MouseButton button);
    Q_INVOKABLE bool submitFields(GameEnums::Promotion promotion = GameEnums::NoPromotion);
    Q_INVOKABLE void cancelPromotion();
    Q_INVOKABLE void undo();
    Q_INVOKABLE bool hint();
    Q_INVOKABLE void finish();
    Q_INVOKABLE bool requestClose();
    void tick();
  signals:
    void pageChanged();
    void busyChanged();
    void promotionRequested();
    void promotionDismissed();
    void closeReady();

  private:
    void refresh();
    void publishStatus();
    void setPage(Page page);
    void invalidate();
    void updateHighlights();
    void searchCompleted();
    void report(Status status);
    std::optional<Session> session_;
    SessionLog log_;
    BoardModel board_;
    HistoryModel history_;
    InputModel input_;
    ClockModel clocks_;
    StatusModel game_;
    SettingsDraft settings_;
    SearchJobs jobs_;
    QTimer timer_;
    SessionState state_{};
    Status diagnostic_ = Status::Ok;
    Page page_ = MainMenu;
    uint64_t generation_ = 0, promotionRevision_ = 0;
    int failedCount_ = -1;
    Color failedTurn_ = Color::Empty;
    bool closing_ = false, promotionPending_ = false;
};
} // namespace ac

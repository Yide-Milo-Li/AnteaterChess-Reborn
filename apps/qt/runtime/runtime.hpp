#pragma once
#include "anteater/session.hpp"
#include <QString>
#include <functional>

namespace ac {
QString executableDirectory();
int64_t monotonicMilliseconds(void *unused);
QString pieceAsset(Piece piece);
bool verifyResources();
bool retiredRuntimeLoaded();

// Empty explicit paths represent discovery failure; no cwd/user-data fallback.
class SessionLog {
  public:
    explicit SessionLog(const QString &directory);
    static SessionLog besideExecutable();
    Status write(const SessionSnapshot &snapshot);
    const QString &path() const {
        return path_;
    }

  private:
    QString directory_, path_;
    uint64_t gameId_ = 0;
};
} // namespace ac

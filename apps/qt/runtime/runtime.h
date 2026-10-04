#pragma once
#include "anteater/session.h"
#include <QString>
#include <functional>

namespace ac {
QString executableDirectory();
int64_t monotonicMilliseconds(void *unused);
QString pieceAsset(AcPiece piece);
bool verifyResources();

// Empty explicit paths represent discovery failure; no cwd/user-data fallback.
class SessionLog {
public:
    explicit SessionLog(const QString &directory);
    static SessionLog besideExecutable();
    static AcStatus writeCallback(void *context, const AcSnapshot *snapshot);
    AcStatus write(const AcSnapshot &snapshot);
    const QString &path() const { return path_; }
private:
    QString directory_, path_;
    uint64_t gameId_ = 0;
};
}

#ifndef ANTEATER_SESSION_H
#define ANTEATER_SESSION_H
#include "anteater/rules.hpp"

namespace ac {
struct Session;
enum class SessionPhase { Idle, Active, Finished };
struct Snapshot {
    Position position;
    GameConfig config;
    SessionPhase phase;
    GameResult result;
    uint64_t revision;
    uint64_t gameId;
    const Move *history; /* borrowed until next session mutation */
    const uint64_t *hashes;
    int historyCount;
    int64_t elapsedMs;
    int remaining[2];
    int tournamentRemainingMs[2];
    Status diagnostic;
};
typedef Status (*LogWrite)(void *context, const Snapshot *snapshot);
struct SessionOptions {
    Clock clock;
    LogWrite log;
    void *logContext;
    void *(*allocate)(void *context, size_t size);
    void (*deallocate)(void *context, void *pointer);
    void *allocatorContext;
};
Session *session_create(const SessionOptions *options);
void session_destroy(Session *session);
Status session_start(Session *session, const GameConfig *config);
Status session_snapshot(const Session *session, Snapshot *out);
Status session_tick(Session *session);
Status session_submit(Session *session, MoveRequest request);
Status session_submit_ai(Session *session, Move move, uint64_t revision, int budgetMs, int elapsedMs);
Status session_undo(Session *session);
Status session_finish(Session *session);
Status session_promotion(const Session *session, MoveRequest request, int *needed);
int session_is_ai(const GameConfig *config, Color color);
int session_ai_budget(const Session *session);
const char *status_message(Status status);

} // namespace ac
#endif

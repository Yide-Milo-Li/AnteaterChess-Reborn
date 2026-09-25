#ifndef ANTEATER_SESSION_H
#define ANTEATER_SESSION_H
#include "anteater/rules.h"
typedef struct AcSession AcSession;
typedef enum { AC_SESSION_IDLE, AC_SESSION_ACTIVE, AC_SESSION_FINISHED } AcSessionPhase;
typedef struct {
    AcPosition position;
    AcGameConfig config;
    AcSessionPhase phase;
    AcGameResult result;
    uint64_t revision;
    uint64_t gameId;
    const AcMove *history; /* borrowed until next session mutation */
    const uint64_t *hashes;
    int historyCount;
    int64_t elapsedMs;
    int remaining[2];
    int tournamentRemainingMs[2];
    AcStatus diagnostic;
} AcSnapshot;
typedef AcStatus (*AcLogWrite)(void *context, const AcSnapshot *snapshot);
typedef struct {
    AcClock clock;
    AcLogWrite log;
    void *logContext;
    void *(*allocate)(void *context, size_t size);
    void (*deallocate)(void *context, void *pointer);
    void *allocatorContext;
} AcSessionOptions;
AcSession *ac_session_create(const AcSessionOptions *options);
void ac_session_destroy(AcSession *session);
AcStatus ac_session_start(AcSession *session, const AcGameConfig *config);
AcStatus ac_session_snapshot(const AcSession *session, AcSnapshot *out);
AcStatus ac_session_tick(AcSession *session);
AcStatus ac_session_submit(AcSession *session, AcMoveRequest request);
AcStatus ac_session_submit_ai(AcSession *session, AcMove move, uint64_t revision, int budgetMs, int elapsedMs);
AcStatus ac_session_undo(AcSession *session);
AcStatus ac_session_finish(AcSession *session);
AcStatus ac_session_promotion(const AcSession *session, AcMoveRequest request, int *needed);
int ac_session_is_ai(const AcGameConfig *config, AcColor color);
int ac_session_ai_budget(const AcSession *session);
const char *ac_status_message(AcStatus status);
#endif

#ifndef ANTEATER_TYPES_H
#define ANTEATER_TYPES_H
#include <stdint.h>
#include <stddef.h>

typedef enum { AC_ANT, AC_ROOK, AC_KNIGHT, AC_BISHOP, AC_QUEEN, AC_KING, AC_ANTEATER, AC_EMPTY_PIECE } AcPieceType;

typedef enum { AC_WHITE, AC_BLACK, AC_EMPTY_COLOR } AcColor;

typedef struct {
    AcPieceType type;
    AcColor color;
} AcPiece;

AcPiece ac_create_piece(AcPieceType type, AcColor color);
int ac_is_same_color(AcPiece a, AcPiece b);
char ac_get_piece_symbol(AcPiece piece);
int ac_is_valid_piece(AcPiece piece);

typedef struct {
    int row;
    int col;
} AcSquare;

AcSquare ac_create_position(int row, int col);
int ac_is_valid_position(AcSquare pos);
AcSquare ac_parse_position(const char *input);
int ac_position_equal(AcSquare a, AcSquare b);

#define AC_ROWS 8
#define AC_COLS 10

typedef struct {
    AcPiece cells[AC_ROWS][AC_COLS];
} AcBoard;

void ac_init_board(AcBoard *board);
AcPiece ac_get_piece(const AcBoard *board, AcSquare pos);
void ac_set_piece(AcBoard *board, AcSquare pos, AcPiece piece);
void ac_remove_piece(AcBoard *board, AcSquare pos);

#define AC_MAX_CHAIN 10

typedef enum {
    AC_NO_SPECIAL_MOVE,
    AC_CASTLING_KINGSIDE,
    AC_CASTLING_QUEENSIDE,
    AC_EN_PASSANT,
    AC_PROMOTION_QUEEN,
    AC_PROMOTION_ROOK,
    AC_PROMOTION_BISHOP,
    AC_PROMOTION_KNIGHT,
    AC_ANTEATER_CAPTURE
} AcSpecialMove;

typedef struct {
    AcSquare pos;
    AcPiece piece;
} AcCaptureRecord;

typedef struct {
    AcSquare from;
    AcSquare to;
    AcPiece movedPiece;
    AcSquare path[AC_MAX_CHAIN];
    int pathLength;
    AcCaptureRecord captures[AC_MAX_CHAIN];
    int captureCount;
    AcSpecialMove specialType;
} AcMove;

AcMove ac_create_move(AcSquare from, AcSquare to, AcPiece piece);
void ac_add_capture(AcMove *move, AcSquare pos, AcPiece piece);
void ac_add_path_step(AcMove *move, AcSquare pos);
void ac_set_special_move(AcMove *move, AcSpecialMove type);
int ac_is_promotion_special_move(AcSpecialMove type);

typedef enum { AC_MODE_HUMAN_VS_HUMAN, AC_MODE_HUMAN_VS_COMPUTER, AC_MODE_COMPUTER_VS_COMPUTER } AcGameMode;

typedef enum {
    AC_DIFFICULTY_NONE,
    AC_DIFFICULTY_EASY,
    AC_DIFFICULTY_MEDIUM,
    AC_DIFFICULTY_HARD,
    AC_DIFFICULTY_EXPERIMENTAL,
    AC_DIFFICULTY_TOURNAMENT
} AcAIDifficulty;

typedef struct {
    AcGameMode mode;
    AcColor playerColor;
    AcAIDifficulty aiDifficultyWhite;
    AcAIDifficulty aiDifficultyBlack;
    int timerEnabled;
    int aiTimeLimit;
    int initialTimeSeconds;
} AcGameConfig;

void ac_init_default_game_config(AcGameConfig *config);
void ac_init_game_config_for_mode(AcGameConfig *config, AcGameMode mode);
int ac_get_default_ai_time_budget_ms(AcAIDifficulty difficulty);
int ac_get_ai_time_budget_ms(const AcGameConfig *config, AcAIDifficulty difficulty);
int ac_get_required_ai_turn_timer_seconds(const AcGameConfig *config);
int ac_is_ai_turn_timer_setting_valid(const AcGameConfig *config);

typedef enum {
    AC_PROMOTION_CHOICE_NONE,
    AC_PROMOTION_CHOICE_QUEEN,
    AC_PROMOTION_CHOICE_ROOK,
    AC_PROMOTION_CHOICE_BISHOP,
    AC_PROMOTION_CHOICE_KNIGHT
} AcPromotionChoice;

typedef struct {
    AcSquare from;
    AcSquare to;
    AcPromotionChoice promotion;
} AcMoveRequest;

int ac_is_valid_promotion_choice(AcPromotionChoice promotion);
int ac_create_move_request(AcMoveRequest *request, AcSquare from, AcSquare to, AcPromotionChoice promotion);

#define AC_MAX_MOVES 1024
typedef enum {
    AC_OK,
    AC_INVALID_ARGUMENT,
    AC_ILLEGAL_MOVE,
    AC_OUT_OF_MEMORY,
    AC_CAPACITY,
    AC_CANCELLED,
    AC_STALE_RESULT,
    AC_UNAVAILABLE,
    AC_IO_ERROR
} AcStatus;
typedef enum {
    AC_RESULT_NONE,
    AC_RESULT_WHITE_WIN,
    AC_RESULT_BLACK_WIN,
    AC_RESULT_DRAW,
    AC_RESULT_TERMINATED_BY_USER
} AcGameResult;
typedef int64_t (*AcNowMs)(void *context);
typedef struct {
    AcNowMs now;
    void *context;
} AcClock;
typedef struct {
    AcBoard board;
    AcColor currentTurn;
    unsigned char castlingRights;
    AcSquare enPassant; /* capturable ant square, or (-1,-1) */
    int moveCount;
    uint64_t hash;
} AcPosition;
typedef struct {
    AcPosition before;
} AcUndo;
/* Heap-own this buffer in recursive/worker code. Capacity failures are sticky. */
typedef struct {
    AcMove moves[AC_MAX_MOVES];
    int count;
    AcStatus status;
} AcMoveList;
#endif

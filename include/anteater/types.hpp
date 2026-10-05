#ifndef ANTEATER_TYPES_H
#define ANTEATER_TYPES_H
#include <stdint.h>
#include <stddef.h>
#include <array>
#include <type_traits>
#include <variant>
#include <utility>

namespace ac {
enum class Status;
enum class PieceType { Ant, Rook, Knight, Bishop, Queen, King, Anteater, Empty };

enum class Color { White, Black, Empty };

struct Piece {
    PieceType type;
    Color color;
    bool operator==(const Piece &) const = default;
};

Piece create_piece(PieceType type, Color color);
int is_same_color(Piece a, Piece b);
char get_piece_symbol(Piece piece);
int is_valid_piece(Piece piece);

struct Square {
    int row;
    int col;
    bool operator==(const Square &) const = default;
};

Square create_position(int row, int col);
int is_valid_position(Square pos);
Square parse_position(const char *input);
int position_equal(Square a, Square b);

inline constexpr int Rows = 8;
inline constexpr int Columns = 10;

struct Board {
    std::array<std::array<Piece, Columns>, Rows> cells;
    bool operator==(const Board &) const = default;
};

void init_board(Board *board);
Piece get_piece(const Board *board, Square pos);
void set_piece(Board *board, Square pos, Piece piece);
void remove_piece(Board *board, Square pos);

inline constexpr int MaxChain = 10;

enum class SpecialMove {
    None,
    CastlingKingside,
    CastlingQueenside,
    EnPassant,
    PromotionQueen,
    PromotionRook,
    PromotionBishop,
    PromotionKnight,
    AnteaterCapture
};

struct CaptureRecord {
    Square pos;
    Piece piece;
    bool operator==(const CaptureRecord &) const = default;
};

struct Move {
    Square from;
    Square to;
    Piece movedPiece;
    std::array<Square, MaxChain> path;
    int pathLength;
    std::array<CaptureRecord, MaxChain> captures;
    int captureCount;
    SpecialMove specialType;
    bool operator==(const Move &) const = default;
};

Move create_move(Square from, Square to, Piece piece);
void add_capture(Move *move, Square pos, Piece piece);
void add_path_step(Move *move, Square pos);
void set_special_move(Move *move, SpecialMove type);
int is_promotion_special_move(SpecialMove type);

enum class GameMode { HumanVsHuman, HumanVsComputer, ComputerVsComputer };

enum class Difficulty {
    None = 0,
    Easy = 1,
    Medium = 2,
    Hard = 3,
    /* Value 4 was Experimental and is no longer accepted. */
    Tournament = 5
};

struct GameConfig {
    GameMode mode;
    Color playerColor;
    Difficulty aiDifficultyWhite;
    Difficulty aiDifficultyBlack;
    int timerEnabled;
    int aiTimeLimit;
    int initialTimeSeconds;
    bool operator==(const GameConfig &) const = default;
};

void init_default_game_config(GameConfig *config);
void init_game_config_for_mode(GameConfig *config, GameMode mode);
int get_default_ai_time_budget_ms(Difficulty difficulty);
int get_ai_time_budget_ms(const GameConfig *config, Difficulty difficulty);
int get_required_ai_turn_timer_seconds(const GameConfig *config);
int is_ai_turn_timer_setting_valid(const GameConfig *config);

enum class PromotionChoice { None, Queen, Rook, Bishop, Knight };

struct MoveRequest {
    Square from;
    Square to;
    PromotionChoice promotion;
    bool operator==(const MoveRequest &) const = default;
};

int is_valid_promotion_choice(PromotionChoice promotion);
Status create_move_request(MoveRequest *request, Square from, Square to, PromotionChoice promotion);

inline constexpr int MaxMoves = 1024;
enum class Status {
    Ok,
    InvalidArgument,
    IllegalMove,
    OutOfMemory,
    Capacity,
    Cancelled,
    StaleResult,
    Unavailable,
    IoError
};

// Error metadata never allocates, including when reporting allocation failure.
struct Error {
    Status status;
    const char *message;
};
template <class T> using Result = std::variant<T, Error>;
template <class E>
    requires std::is_enum_v<E>
constexpr auto value(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}
template <class E> constexpr std::size_t enum_index(E e) noexcept {
    return static_cast<std::size_t>(e);
}
constexpr bool operator!(Status status) noexcept {
    return status == Status::Ok;
}

enum class GameResult { None, WhiteWin, BlackWin, Draw, TerminatedByUser };
typedef int64_t (*NowMs)(void *context);
struct Clock {
    NowMs now;
    void *context;
};
struct Position {
    Board board;
    Color currentTurn;
    unsigned char castlingRights;
    Square enPassant; /* capturable ant square, or (-1,-1) */
    int moveCount;
    uint64_t hash;
    bool operator==(const Position &) const = default;
};
struct Undo {
    Position before;
    bool operator==(const Undo &) const = default;
};
/* Heap-own this buffer in recursive/worker code. Capacity failures are sticky. */
struct MoveList {
    std::array<Move, MaxMoves> moves;
    int count;
    Status status;
};

} // namespace ac
#endif

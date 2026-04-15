#include <assert.h>

#include "core/board.h"
#include "core/gameconfig.h"

static void assertPieceEquals(Piece piece, PieceType type, Color color) {
    assert(piece.type == type);
    assert(piece.color == color);
}

static void test_board_initialization(void) {
    static const PieceType backRank[COLS] = {
        ROOK, KNIGHT, BISHOP, ANTEATER, QUEEN,
        KING, ANTEATER, BISHOP, KNIGHT, ROOK
    };
    Board board;
    int col;

    initBoard(&board);

    for (col = 0; col < COLS; ++col) {
        assertPieceEquals(getPiece(&board, createPosition(0, col)), backRank[col], BLACK);
        assertPieceEquals(getPiece(&board, createPosition(1, col)), ANT, BLACK);
        assertPieceEquals(getPiece(&board, createPosition(6, col)), ANT, WHITE);
        assertPieceEquals(getPiece(&board, createPosition(7, col)), backRank[col], WHITE);
    }

    assertPieceEquals(getPiece(&board, createPosition(3, 4)), EMPTY_PIECE, EMPTY_COLOR);
    assertPieceEquals(getPiece(&board, createPosition(4, 9)), EMPTY_PIECE, EMPTY_COLOR);
}

static void test_board_mutation(void) {
    Board board;
    Position pos = createPosition(3, 3);
    Piece queen = createPiece(QUEEN, WHITE);

    initBoard(&board);
    setPiece(&board, pos, queen);
    assertPieceEquals(getPiece(&board, pos), QUEEN, WHITE);

    removePiece(&board, pos);
    assertPieceEquals(getPiece(&board, pos), EMPTY_PIECE, EMPTY_COLOR);
}

static void test_invalid_position_access(void) {
    Board board;
    Position invalid = createPosition(-1, 20);

    initBoard(&board);
    setPiece(&board, invalid, createPiece(KING, BLACK));
    removePiece(&board, invalid);
    assertPieceEquals(getPiece(&board, invalid), EMPTY_PIECE, EMPTY_COLOR);
}

static void test_default_gameconfig(void) {
    GameConfig config;

    initDefaultGameConfig(&config);
    assert(config.mode == MODE_HUMAN_VS_HUMAN);
    assert(config.playerColor == WHITE);
    assert(config.aiDifficultyWhite == DIFFICULTY_NONE);
    assert(config.aiDifficultyBlack == DIFFICULTY_NONE);
    assert(config.timerEnabled == 0);
    assert(config.aiTimeLimit == 0);
    assert(config.initialTimeSeconds == 0);
}

int main(void) {
    test_board_initialization();
    test_board_mutation();
    test_invalid_position_access();
    test_default_gameconfig();
    return 0;
}

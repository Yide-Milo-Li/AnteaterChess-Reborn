#include "anteater/rules.h"
#include <assert.h>

static void assertPieceEquals(AcPiece piece, AcPieceType type, AcColor color) {
    assert(piece.type == type);
    assert(piece.color == color);
}

static void test_board_initialization(void) {
    static const AcPieceType backRank[AC_COLS] = {AC_ROOK, AC_KNIGHT,   AC_BISHOP, AC_ANTEATER, AC_QUEEN,
                                                  AC_KING, AC_ANTEATER, AC_BISHOP, AC_KNIGHT,   AC_ROOK};
    AcBoard board;
    int col;

    ac_init_board(&board);

    for (col = 0; col < AC_COLS; ++col) {
        assertPieceEquals(ac_get_piece(&board, ac_create_position(0, col)), backRank[col], AC_BLACK);
        assertPieceEquals(ac_get_piece(&board, ac_create_position(1, col)), AC_ANT, AC_BLACK);
        assertPieceEquals(ac_get_piece(&board, ac_create_position(6, col)), AC_ANT, AC_WHITE);
        assertPieceEquals(ac_get_piece(&board, ac_create_position(7, col)), backRank[col], AC_WHITE);
    }

    assertPieceEquals(ac_get_piece(&board, ac_create_position(3, 4)), AC_EMPTY_PIECE, AC_EMPTY_COLOR);
    assertPieceEquals(ac_get_piece(&board, ac_create_position(4, 9)), AC_EMPTY_PIECE, AC_EMPTY_COLOR);
}

static void test_board_mutation(void) {
    AcBoard board;
    AcSquare pos = ac_create_position(3, 3);
    AcPiece queen = ac_create_piece(AC_QUEEN, AC_WHITE);

    ac_init_board(&board);
    ac_set_piece(&board, pos, queen);
    assertPieceEquals(ac_get_piece(&board, pos), AC_QUEEN, AC_WHITE);

    ac_remove_piece(&board, pos);
    assertPieceEquals(ac_get_piece(&board, pos), AC_EMPTY_PIECE, AC_EMPTY_COLOR);
}

static void test_invalid_position_access(void) {
    AcBoard board;
    AcSquare invalid = ac_create_position(-1, 20);

    ac_init_board(&board);
    ac_set_piece(&board, invalid, ac_create_piece(AC_KING, AC_BLACK));
    ac_remove_piece(&board, invalid);
    assertPieceEquals(ac_get_piece(&board, invalid), AC_EMPTY_PIECE, AC_EMPTY_COLOR);
}

static void test_default_gameconfig(void) {
    AcGameConfig config;

    ac_init_default_game_config(&config);
    assert(config.mode == AC_MODE_HUMAN_VS_HUMAN);
    assert(config.playerColor == AC_WHITE);
    assert(config.aiDifficultyWhite == AC_DIFFICULTY_NONE);
    assert(config.aiDifficultyBlack == AC_DIFFICULTY_NONE);
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

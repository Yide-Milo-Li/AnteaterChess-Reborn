#include "anteater/rules.hpp"
#include <assert.h>

using namespace ac;

static void assertPieceEquals(Piece piece, PieceType type, Color color) {
    assert(piece.type == type);
    assert(piece.color == color);
}

static void test_board_initialization(void) {
    static const PieceType backRank[Columns] = {
        PieceType::Rook, PieceType::Knight,   PieceType::Bishop, PieceType::Anteater, PieceType::Queen,
        PieceType::King, PieceType::Anteater, PieceType::Bishop, PieceType::Knight,   PieceType::Rook};
    Board board;
    int col;

    init_board(&board);

    for (col = 0; col < Columns; ++col) {
        assertPieceEquals(get_piece(&board, create_position(0, col)), backRank[col], Color::Black);
        assertPieceEquals(get_piece(&board, create_position(1, col)), PieceType::Ant, Color::Black);
        assertPieceEquals(get_piece(&board, create_position(6, col)), PieceType::Ant, Color::White);
        assertPieceEquals(get_piece(&board, create_position(7, col)), backRank[col], Color::White);
    }

    assertPieceEquals(get_piece(&board, create_position(3, 4)), PieceType::Empty, Color::Empty);
    assertPieceEquals(get_piece(&board, create_position(4, 9)), PieceType::Empty, Color::Empty);
}

static void test_board_mutation(void) {
    Board board;
    Square pos = create_position(3, 3);
    Piece queen = create_piece(PieceType::Queen, Color::White);

    init_board(&board);
    set_piece(&board, pos, queen);
    assertPieceEquals(get_piece(&board, pos), PieceType::Queen, Color::White);

    remove_piece(&board, pos);
    assertPieceEquals(get_piece(&board, pos), PieceType::Empty, Color::Empty);
}

static void test_invalid_position_access(void) {
    Board board;
    Square invalid = create_position(-1, 20);

    init_board(&board);
    set_piece(&board, invalid, create_piece(PieceType::King, Color::Black));
    remove_piece(&board, invalid);
    assertPieceEquals(get_piece(&board, invalid), PieceType::Empty, Color::Empty);
}

static void test_default_gameconfig(void) {
    GameConfig config;

    init_default_game_config(&config);
    assert(config.mode == GameMode::HumanVsHuman);
    assert(config.playerColor == Color::White);
    assert(config.aiDifficultyWhite == Difficulty::None);
    assert(config.aiDifficultyBlack == Difficulty::None);
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

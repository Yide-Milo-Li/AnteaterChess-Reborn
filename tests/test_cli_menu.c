#include <assert.h>
#include <stdio.h>

#include "cli/cli_menu.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"

#define CLI_FIXTURE_DIR "tests/fixtures/cli/"

/* Replace stdin with a deterministic fixture file for one CLI prompt test. */
static void writeFixtureAndRedirect(const char *path, const char *contents) {
    FILE *file = fopen(path, "w");

    assert(file != NULL);
    fputs(contents, file);
    fclose(file);
    assert(freopen(path, "r", stdin) != NULL);
}

/* Check that the main menu reprompts until it receives a valid choice. */
static void test_main_menu_selection(void) {
    int selection;

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "main_menu.txt", "9\n1\n");
    assert(cliGetMainMenuSelection(&selection) == 0);
    assert(selection == 1);
}

/* Check that the game mode menu accepts the documented five options. */
static void test_game_mode_selection(void) {
    int selection;

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "game_mode.txt", "0\n5\n");
    assert(cliGetGameModeSelection(&selection) == 0);
    assert(selection == 5);
}

/* Check that human-vs-human setup keeps all AI fields disabled. */
static void test_game_setup_human_vs_human(void) {
    GameConfig config;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_HUMAN;
    writeFixtureAndRedirect(CLI_FIXTURE_DIR "setup_no_timer.txt", "2\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.mode == MODE_HUMAN_VS_HUMAN);
    assert(config.aiDifficultyWhite == DIFFICULTY_NONE);
    assert(config.aiDifficultyBlack == DIFFICULTY_NONE);
    assert(config.aiTimeLimit == 0);
    assert(config.timerEnabled == 0);
    assert(config.initialTimeSeconds == 0);

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_HUMAN;
    writeFixtureAndRedirect(CLI_FIXTURE_DIR "setup_with_timer.txt", "1\n45\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.mode == MODE_HUMAN_VS_HUMAN);
    assert(config.aiDifficultyWhite == DIFFICULTY_NONE);
    assert(config.aiDifficultyBlack == DIFFICULTY_NONE);
    assert(config.timerEnabled == 1);
    assert(config.initialTimeSeconds == 45);
}

/* Check that human-vs-computer setup seeds roles and AI settings correctly. */
static void test_game_setup_human_vs_computer(void) {
    GameConfig config;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    writeFixtureAndRedirect(CLI_FIXTURE_DIR "setup_no_timer.txt", "2\n3\n1\n2\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.mode == MODE_HUMAN_VS_COMPUTER);
    assert(config.playerColor == BLACK);
    assert(config.aiDifficultyWhite == DIFFICULTY_HARD);
    assert(config.aiDifficultyBlack == DIFFICULTY_NONE);
    assert(config.aiTimeLimit == 1);
    assert(config.timerEnabled == 0);
}

/* Check that computer-vs-computer setup accepts separate AI settings per side. */
static void test_game_setup_computer_vs_computer(void) {
    GameConfig config;

    initDefaultGameConfig(&config);
    config.mode = MODE_COMPUTER_VS_COMPUTER;
    writeFixtureAndRedirect(CLI_FIXTURE_DIR "setup_with_timer.txt", "1\n2\n1\n2\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.mode == MODE_COMPUTER_VS_COMPUTER);
    assert(config.aiDifficultyWhite == DIFFICULTY_EASY);
    assert(config.aiDifficultyBlack == DIFFICULTY_MEDIUM);
    assert(config.aiTimeLimit == 1);
    assert(config.timerEnabled == 0);
}

/* Check that AI setup fields re-prompt after invalid values. */
static void test_game_setup_reprompts_invalid_ai_values(void) {
    GameConfig config;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    writeFixtureAndRedirect(CLI_FIXTURE_DIR "game_mode.txt", "3\n1\n0\n2\n0\n1\n2\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.playerColor == WHITE);
    assert(config.aiDifficultyBlack == DIFFICULTY_MEDIUM);
    assert(config.aiTimeLimit == 1);
}

/* Check that the end-game menu returns the validated selection. */
static void test_endgame_menu_selection(void) {
    GameConfig config;
    GameState state;
    int selection;

    initDefaultGameConfig(&config);
    initGameState(&state, &config);
    state.result = RESULT_WHITE_WIN;
    state.gameOver = 1;

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "endgame_menu.txt", "3\n");
    assert(cliShowEndGameMenu(&state, &selection) == 0);
    assert(selection == 3);
}

/* Run the CLI menu/input regression suite. */
int main(void) {
    test_main_menu_selection();
    test_game_mode_selection();
    test_game_setup_human_vs_human();
    test_game_setup_human_vs_computer();
    test_game_setup_computer_vs_computer();
    test_game_setup_reprompts_invalid_ai_values();
    test_endgame_menu_selection();
    return 0;
}

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

/* Check that game setup can build timer-off and timer-on configurations. */
static void test_game_setup_config(void) {
    GameConfig config;

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "setup_no_timer.txt", "2\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.mode == MODE_HUMAN_VS_HUMAN);
    assert(config.timerEnabled == 0);
    assert(config.initialTimeSeconds == 0);

    writeFixtureAndRedirect(CLI_FIXTURE_DIR "setup_with_timer.txt", "1\n45\n");
    assert(cliGetGameSetupConfig(&config) == 0);
    assert(config.mode == MODE_HUMAN_VS_HUMAN);
    assert(config.timerEnabled == 1);
    assert(config.initialTimeSeconds == 45);
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
    test_game_setup_config();
    test_endgame_menu_selection();
    return 0;
}

#include "gui_internal.h"

#include <ctype.h>
#include <stdio.h>

int gui_current_turn_is_ai(const GuiView *state) {
    if (state == NULL || state->currentTurn < AC_WHITE || state->currentTurn > AC_BLACK) {
        return 0;
    }

    return state->players[state->currentTurn].type == AC_AI;
}

const char *gui_game_mode_title(AcGameMode mode) {
    switch (mode) {
    case AC_MODE_HUMAN_VS_HUMAN:
        return "Human vs. Human";
    case AC_MODE_HUMAN_VS_COMPUTER:
        return "Human vs. Computer";
    case AC_MODE_COMPUTER_VS_COMPUTER:
        return "Computer vs. Computer";
    default:
        return "Game Setup";
    }
}

const char *gui_game_result_text(AcGameResult result) {
    switch (result) {
    case AC_RESULT_WHITE_WIN:
        return "White wins.";
    case AC_RESULT_BLACK_WIN:
        return "Black wins.";
    case AC_RESULT_DRAW:
        return "Draw.";
    case AC_RESULT_TERMINATED_BY_USER:
        return "Game ended by user.";
    case AC_RESULT_NONE:
    default:
        return "Game over.";
    }
}

AcAIDifficulty gui_difficulty_from_index(int index) {
    switch (index) {
    case 0:
        return AC_DIFFICULTY_EASY;
    case 1:
        return AC_DIFFICULTY_MEDIUM;
    case 2:
        return AC_DIFFICULTY_HARD;
    case 3:
        return AC_DIFFICULTY_TOURNAMENT;
    case 4:
        return AC_DIFFICULTY_EXPERIMENTAL;
    default:
        return AC_DIFFICULTY_EASY;
    }
}

int gui_difficulty_index(AcAIDifficulty difficulty) {
    switch (difficulty) {
    case AC_DIFFICULTY_MEDIUM:
        return 1;
    case AC_DIFFICULTY_HARD:
        return 2;
    case AC_DIFFICULTY_TOURNAMENT:
        return 3;
    case AC_DIFFICULTY_EXPERIMENTAL:
        return 4;
    case AC_DIFFICULTY_EASY:
    case AC_DIFFICULTY_NONE:
    default:
        return 0;
    }
}

void gui_format_elapsed_text(char buffer[32], int64_t elapsedSeconds) {
    int hours;
    int minutes;
    int seconds;

    if (buffer == NULL) {
        return;
    }

    if (elapsedSeconds < 0) {
        elapsedSeconds = 0;
    }

    hours = (int)(elapsedSeconds / 3600);
    minutes = (int)((elapsedSeconds % 3600) / 60);
    seconds = (int)(elapsedSeconds % 60);
    snprintf(buffer, 32, "%02d:%02d:%02d", hours, minutes, seconds);
}

void gui_format_timer_text(char buffer[32], const char *prefix, int seconds) {
    int hours;
    int minutes;
    int remainderSeconds;

    if (buffer == NULL || prefix == NULL) {
        return;
    }

    if (seconds < 0) {
        snprintf(buffer, 32, "%s --:--:--", prefix);
        return;
    }

    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    remainderSeconds = seconds % 60;
    snprintf(buffer, 32, "%s %02d:%02d:%02d", prefix, hours, minutes, remainderSeconds);
}

void gui_format_position_text(AcSquare pos, char buffer[8]) {
    if (buffer == NULL) {
        return;
    }

    if (!ac_is_valid_position(pos)) {
        snprintf(buffer, 8, "??");
        return;
    }

    buffer[0] = (char)('A' + pos.col);
    buffer[1] = (char)('0' + (8 - pos.row));
    buffer[2] = '\0';
}

void gui_format_hint_text(AcMove move, char buffer[64]) {
    char fromText[8];
    char toText[8];

    if (buffer == NULL) {
        return;
    }

    gui_format_position_text(move.from, fromText);
    gui_format_position_text(move.to, toText);
    snprintf(buffer, 64, "Hint: %s -> %s", fromText, toText);
}

static int gui_piece_asset_index(AcPiece piece, int *colorIndex, int *typeIndex) {
    if (colorIndex == NULL || typeIndex == NULL || !ac_is_valid_piece(piece) || piece.type == AC_EMPTY_PIECE) {
        return 0;
    }

    if (piece.color == AC_WHITE) {
        *colorIndex = 0;
    } else if (piece.color == AC_BLACK) {
        *colorIndex = 1;
    } else {
        return 0;
    }

    *typeIndex = piece.type - AC_ANT;
    return *typeIndex >= 0 && *typeIndex < 7;
}

static const char *gui_get_piece_asset_filename(AcPiece piece) {
    static const char *whiteAssets[] = {"WhiteAntsvg.svg", "WhiteRook.svg", "WhiteKnight.svg",  "WhiteBishop.svg",
                                        "WhiteQueen.svg",  "WhiteKing.svg", "WhiteAnteater.svg"};
    static const char *blackAssets[] = {"BlackAnt.svg",   "BlackRook.svg", "BlackKnight.svg",  "BlackBishop.svg",
                                        "BlackQueen.svg", "BlackKing.svg", "BlackAnteater.svg"};
    int colorIndex;
    int typeIndex;

    if (!gui_piece_asset_index(piece, &colorIndex, &typeIndex)) {
        return NULL;
    }

    return colorIndex == 0 ? whiteAssets[typeIndex] : blackAssets[typeIndex];
}

const char *gui_get_piece_asset_path(AcPiece piece) {
    static char paths[2][7][64];
    const char *filename;
    int colorIndex;
    int typeIndex;

    filename = gui_get_piece_asset_filename(piece);
    if (filename == NULL || !gui_piece_asset_index(piece, &colorIndex, &typeIndex)) {
        return NULL;
    }

    snprintf(paths[colorIndex][typeIndex], sizeof(paths[colorIndex][typeIndex]), "/org/anteater/reborn/%s", filename);
    return paths[colorIndex][typeIndex];
}

static GdkPixbuf *gui_load_asset_pixbuf(const char *path, int size) {
    if (!path || size <= 0)
        return NULL;
    GError *error = NULL;
    GdkPixbuf *p = gdk_pixbuf_new_from_resource_at_scale(path, size, size, TRUE, &error);
    if (error) {
        g_warning("Resource %s: %s",path,error->message);
        g_error_free(error);
    }
    return p;
}

GdkPixbuf *gui_get_piece_pixbuf(AcPiece piece, int size) {
    static GdkPixbuf *cache[2][7];
    static int cachedSize = 0;
    const char *assetPath;
    int colorIndex;
    int typeIndex;
    int row;
    int col;

    if (!gui_piece_asset_index(piece, &colorIndex, &typeIndex) || size <= 0) {
        return NULL;
    }

    if (cachedSize != size) {
        for (row = 0; row < 2; ++row) {
            for (col = 0; col < 7; ++col) {
                if (cache[row][col] != NULL) {
                    g_object_unref(cache[row][col]);
                    cache[row][col] = NULL;
                }
            }
        }
        cachedSize = size;
    }

    if (cache[colorIndex][typeIndex] == NULL) {
        assetPath = gui_get_piece_asset_path(piece);
        cache[colorIndex][typeIndex] = gui_load_asset_pixbuf(assetPath, size);
    }

    return cache[colorIndex][typeIndex];
}

GdkPixbuf *gui_get_ui_icon_pixbuf(const char *filename, int size) {
    char assetPath[128];

    if (filename == NULL || size <= 0) {
        return NULL;
    }

    snprintf(assetPath, sizeof(assetPath), "/org/anteater/reborn/%s", filename);
    return gui_load_asset_pixbuf(assetPath, size);
}

GtkWidget *gui_create_ui_icon(const char *filename, int size) {
    GdkPixbuf *pixbuf;
    GtkWidget *image;

    pixbuf = gui_get_ui_icon_pixbuf(filename, size);
    if (pixbuf == NULL) {
        return NULL;
    }

    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    return image;
}

void gui_set_button_icon(GtkWidget *button, const char *filename, int size) {
    GtkWidget *image;

    if (!GTK_IS_BUTTON(button)) {
        return;
    }

    image = gui_create_ui_icon(filename, size);
    if (image == NULL) {
        return;
    }

    gtk_button_set_image(GTK_BUTTON(button), image);
    gtk_button_set_image_position(GTK_BUTTON(button), GTK_POS_LEFT);
    gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
}

void gui_format_piece_fallback_text(AcPiece piece, char buffer[4]) {
    char symbol;

    if (buffer == NULL) {
        return;
    }

    if (!ac_is_valid_piece(piece) || piece.type == AC_EMPTY_PIECE) {
        buffer[0] = '\0';
        return;
    }

    symbol = ac_get_piece_symbol(piece);
    if (piece.color == AC_BLACK) {
        symbol = (char)tolower((unsigned char)symbol);
    }

    buffer[0] = symbol;
    buffer[1] = '\0';
}

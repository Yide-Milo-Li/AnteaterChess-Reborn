#include "gui_internal.h"

#include <ctype.h>
#include <stdio.h>

int gui_current_turn_is_ai(const GameState *state) {
    if (state == NULL || state->currentTurn < WHITE || state->currentTurn > BLACK) {
        return 0;
    }

    return state->players[state->currentTurn].type == AI;
}

const char *gui_game_mode_title(GameMode mode) {
    switch (mode) {
        case MODE_HUMAN_VS_HUMAN:
            return "Human vs. Human";
        case MODE_HUMAN_VS_COMPUTER:
            return "Human vs. Computer";
        case MODE_COMPUTER_VS_COMPUTER:
            return "Computer vs. Computer";
        default:
            return "Game Setup";
    }
}

const char *gui_game_result_text(GameResult result) {
    switch (result) {
        case RESULT_WHITE_WIN:
            return "White wins.";
        case RESULT_BLACK_WIN:
            return "Black wins.";
        case RESULT_DRAW:
            return "Draw.";
        case RESULT_TERMINATED_BY_USER:
            return "Game ended by user.";
        case RESULT_NONE:
        default:
            return "Game over.";
    }
}

AIDifficulty gui_difficulty_from_index(int index) {
    switch (index) {
        case 0:
            return DIFFICULTY_EASY;
        case 1:
            return DIFFICULTY_MEDIUM;
        case 2:
            return DIFFICULTY_HARD;
        default:
            return DIFFICULTY_EASY;
    }
}

int gui_difficulty_index(AIDifficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_MEDIUM:
            return 1;
        case DIFFICULTY_HARD:
        case DIFFICULTY_EXPERIMENTAL:
            return 2;
        case DIFFICULTY_EASY:
        case DIFFICULTY_NONE:
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

    hours = (int) (elapsedSeconds / 3600);
    minutes = (int) ((elapsedSeconds % 3600) / 60);
    seconds = (int) (elapsedSeconds % 60);
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

void gui_format_position_text(Position pos, char buffer[8]) {
    if (buffer == NULL) {
        return;
    }

    if (!isValidPosition(pos)) {
        snprintf(buffer, 8, "??");
        return;
    }

    buffer[0] = (char) ('A' + pos.col);
    buffer[1] = (char) ('0' + (8 - pos.row));
    buffer[2] = '\0';
}

void gui_format_hint_text(Move move, char buffer[64]) {
    char fromText[8];
    char toText[8];

    if (buffer == NULL) {
        return;
    }

    gui_format_position_text(move.from, fromText);
    gui_format_position_text(move.to, toText);
    snprintf(buffer, 64, "Hint: %s -> %s", fromText, toText);
}

static int gui_piece_asset_index(Piece piece, int *colorIndex, int *typeIndex) {
    if (colorIndex == NULL || typeIndex == NULL
        || !isValidPiece(piece) || piece.type == EMPTY_PIECE) {
        return 0;
    }

    if (piece.color == WHITE) {
        *colorIndex = 0;
    } else if (piece.color == BLACK) {
        *colorIndex = 1;
    } else {
        return 0;
    }

    *typeIndex = piece.type - ANT;
    return *typeIndex >= 0 && *typeIndex < 7;
}

static const char *gui_get_piece_asset_filename(Piece piece) {
    static const char *whiteAssets[] = {
        "WhiteAntsvg.svg",
        "WhiteRook.svg",
        "WhiteKnight.svg",
        "WhiteBishop.svg",
        "WhiteQueen.svg",
        "WhiteKing.svg",
        "WhiteAnteater.svg"
    };
    static const char *blackAssets[] = {
        "BlackAnt.svg",
        "BlackRook.svg",
        "BlackKnight.svg",
        "BlackBishop.svg",
        "BlackQueen.svg",
        "BlackKing.svg",
        "BlackAnteater.svg"
    };
    int colorIndex;
    int typeIndex;

    if (!gui_piece_asset_index(piece, &colorIndex, &typeIndex)) {
        return NULL;
    }

    return colorIndex == 0 ? whiteAssets[typeIndex] : blackAssets[typeIndex];
}

const char *gui_get_piece_asset_path(Piece piece) {
    static char paths[2][7][64];
    const char *filename;
    int colorIndex;
    int typeIndex;

    filename = gui_get_piece_asset_filename(piece);
    if (filename == NULL || !gui_piece_asset_index(piece, &colorIndex, &typeIndex)) {
        return NULL;
    }

    snprintf(paths[colorIndex][typeIndex],
        sizeof(paths[colorIndex][typeIndex]),
        "assets/%s",
        filename);
    return paths[colorIndex][typeIndex];
}

static GdkPixbuf *gui_load_asset_pixbuf(const char *assetPath, int size) {
    static const char *prefixes[] = {"", "../", "../../"};
    GdkPixbuf *pixbuf;
    GError *error;
    int index;

    if (assetPath == NULL || size <= 0) {
        return NULL;
    }

    for (index = 0; index < (int)(sizeof(prefixes) / sizeof(prefixes[0])); ++index) {
        char *path = g_strdup_printf("%s%s", prefixes[index], assetPath);

        if (path == NULL) {
            continue;
        }

        error = NULL;
        pixbuf = gdk_pixbuf_new_from_file_at_scale(path, size, size, TRUE, &error);
        g_free(path);
        if (pixbuf != NULL) {
            return pixbuf;
        }
        if (error != NULL) {
            g_error_free(error);
        }
    }

    return NULL;
}

GdkPixbuf *gui_get_piece_pixbuf(Piece piece, int size) {
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

    snprintf(assetPath, sizeof(assetPath), "assets/%s", filename);
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

void gui_format_piece_fallback_text(Piece piece, char buffer[4]) {
    char symbol;

    if (buffer == NULL) {
        return;
    }

    if (!isValidPiece(piece) || piece.type == EMPTY_PIECE) {
        buffer[0] = '\0';
        return;
    }

    symbol = getPieceSymbol(piece);
    if (piece.color == BLACK) {
        symbol = (char)tolower((unsigned char)symbol);
    }

    buffer[0] = symbol;
    buffer[1] = '\0';
}

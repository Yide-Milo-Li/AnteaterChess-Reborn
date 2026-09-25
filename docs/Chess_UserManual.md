# AnteaterChess Reborn user manual

## Install and launch

Supported release targets are Windows x64 and Ubuntu 24.04 x64. Extract the complete platform archive. On Windows launch `anteater-chess.exe`; on Ubuntu install `libgtk-3-0t64 librsvg2-common` with apt and launch `./anteater-chess`. No MSYS2 installation is needed for the Windows runtime package. macOS is not a supported release target. Building from source is described in [Development](DEVELOPMENT.md).

Pieces and icons are compiled into the program. Launching from another working directory is supported. Keep the Windows runtime libraries and their `lib`, `share`, and configuration directories with the executable.

## Start a game

Select New Game, choose a mode, configure players and timing, then start.

| Mode | Behavior |
| --- | --- |
| Human vs Human | Two local players share the board; White starts. |
| Human vs AI | Choose your color and the opponent's difficulty. AI moves automatically on its turn. |
| AI vs AI | Choose a difficulty for each color and watch automatic play. |

The back rank, from A through J for either color, is Rook, Knight, Bishop, Anteater, Queen, King, Anteater, Bishop, Knight, Rook. Ten Ants occupy the next rank. White begins on ranks 1–2 and Black on 7–8.

## Coordinates and input

Files are A–J; ranks are 1–8. A1 is White's left corner; J8 is Black's right corner from White's viewpoint. Internally row 0 is rank 8. Click a friendly piece and then a highlighted destination, or enter From and To coordinates and submit. Typed input accepts lowercase and surrounding whitespace. Illegal selections, blocked moves, and moves leaving your king in check are rejected. F11 toggles fullscreen; Escape leaves fullscreen.

## Pieces and special moves

| Piece | Movement |
| --- | --- |
| Ant | One square forward into an empty square; two from its starting rank if both squares are empty; captures one square diagonally forward. |
| Rook | Any unobstructed distance horizontally or vertically. |
| Bishop | Any unobstructed distance diagonally. |
| Queen | Rook and bishop movement combined. |
| Knight | Two squares along one axis and one along the other; jumps over pieces. |
| King | One adjacent square, avoiding attack. Kings are checkmated, never captured. |
| Anteater | One adjacent empty square, or captures an adjacent enemy Ant and may continue capturing orthogonally adjacent enemy Ants. It cannot capture any other piece and does not attack the king. |

An Anteater's first capture can be diagonal or orthogonal; subsequent captures can turn but must be orthogonal. A chain stops after at most ten captures. Each available stopping point forms a candidate move. If different chains reach the same destination, coordinate input is ambiguous and is rejected; the interface has no path selector. AI can select a complete unambiguous move record internally.

**Castling:** from F1/F8 the king moves to H1/H8 (kingside) with the J-rook moving to G, or to D1/D8 (queenside) with the A-rook moving to E. King and chosen rook must retain their rights; intervening squares must be empty, and the king may not castle out of, through, or into check.

**En passant:** immediately after an enemy Ant advances two squares beside your Ant, your Ant can capture it by moving diagonally into the empty square behind it. This implementation preserves the previous move's en-passant opportunity across a timer-only skipped turn; a subsequent played move replaces it.

**Promotion:** an Ant reaching the far rank promotes to Queen, Rook, Bishop, or Knight, including on a capture. The GUI asks for a choice. Requests without an explicit choice default to Queen. Anteater promotion is unavailable.

## Difficulty and clocks

Easy uses a 350 ms default search budget, Medium 2,200 ms, and Hard 7,000 ms. Experimental currently uses the same search as Hard; there is no plugin discovery. The optional AI time override is expressed in seconds. Depth limits and early completed searches can finish before the allotted budget.

Tournament preserves the original per-color total of 600,999 ms, a 7,000 ms base allocation, and a pool of saved time. Its budget is base plus one quarter of that pool, capped at 10,000 ms, with a 30,000 ms reserve and a 180,000 ms saved-time pool cap. Exhausting the Tournament total awards the win to the opponent. Undo does not refund Tournament time.

The optional **turn timer** counts down for the active side. After a move or undo, the next turn gets the full configured length. When it expires, the turn is skipped without a move-history entry. It is not a game-loss clock. AI configurations require enough turn time for the configured AI budget plus scheduling margin; invalid settings are rejected.

## Hints, undo, and ending

Hints run in the background on human turns, show From/To text and highlight the suggestion; they do not play the move. AI/hints use a snapshot. New games, undo, timeout, navigation, and shutdown invalidate old results. The interface stays responsive while thinking.

Human vs Human undo removes two half-moves, or one if only one exists. Human vs AI rolls history back to the human's turn using the retained history parity policy. AI vs AI removes one half-move. Undo is unavailable with no history or after a finished game. Timer-only skipped turns are not recorded and therefore cannot themselves be undone.

Checkmate wins; stalemate draws. The retained simplified insufficient-material detector also draws king-only/Anteater-only material, a single bishop or knight (possibly with Anteaters), two knights with no bishop or Anteater, and bishops all on one square color with no knights; any Ant, Rook, or Queen prevents that material draw. Threefold repetition is automatically adjudicated only in AI vs AI. There is no fifty-move rule. The session stores at most 1,024 played half-moves; a further submission ends as a draw without writing past capacity. End Game terminates the game voluntarily. New Game starts fresh.

## Logs and troubleshooting

Logs are per-session text files under `AnteaterChess-Reborn/logs` in the user's data directory on Windows (normally `%LOCALAPPDATA%`), or state directory on Linux (`$XDG_STATE_HOME`, normally `~/.local/state`). They include configuration, moves, capture coordinates, elapsed time, and result. A write failure reports a diagnostic while the accepted game action remains applied. Old `bin/logs` files are not imported.

- Missing DLL: re-extract the whole Windows archive; do not copy the executable alone.
- No display on Linux: run in a graphical session; automated tests use Xvfb.
- Invalid timer: increase the turn duration or disable it.
- Move rejected: verify side to move, blockers, check, and Anteater path ambiguity.
- Hint unavailable: wait for the AI/previous hint, return to a human turn, and ensure the game is active.
- Unwritable log: check the user data directory's permissions; no administrator access is required.

There is no networking or saved-game import/export. The [historical manual](legacy/Chess_UserManual.pdf) is the original course submission; its standard-chess wording, paths, and old architecture are not authoritative for Reborn.

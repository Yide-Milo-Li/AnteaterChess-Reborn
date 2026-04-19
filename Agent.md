Overview

This document defines the system agents, their responsibilities, execution behavior, and mappings to header files in the Anteater Chess system.

The system follows a modular, event-driven architecture where all interactions are processed through:

GameState (single source of truth)
Event system (interaction abstraction)
Controller/FSM (execution engine)
Core Architecture Principles
Single Source of Truth
GameState stores all runtime data.
Event-Driven Execution
All interactions are converted into Events and processed by the Controller.
Deterministic Execution
System behavior is fully determined by GameState + Event.
Layered Design
UI → Input → Event → Controller → Rules → Board/Data
Agent Mapping to Header Files

3.1 Controller Agent

Headers:

controller.h
fsm.h

Responsibilities:

Main execution loop
Event polling and dispatch
State transitions
System coordination

APIs:

void runGameLoop(GameState *state);
Event pollEvent();
void processEvent(GameState *state, Event event);
void transitionState(GameState *state, SystemState newState);

Trigger:

Runs continuously during game session
Activated after system initialization

3.2 Event System Agent

Headers:

event.h
event_queue.h

Responsibilities:

Unified event abstraction
Multi-queue event buffering
Decoupling modules

APIs:

int enqueueEvent(EventQueue *q, Event e, QueueType type);
Event dequeueGameplayEvent(EventQueue *q);
Event dequeueSystemEvent(EventQueue *q);
Event dequeueControlEvent(EventQueue *q);

Trigger:

Called whenever input, AI, or system generates an event

3.3 Input Agent

Headers:

input.h

Responsibilities:

Capture user input
Convert input into Command

APIs:

int getMoveInput(Command *cmd);
int getBoardInput(Position *pos, InputType *type);

Trigger:

Called during INPUT phase of controller loop

3.4 UI Agent

Headers:

ui.h

Responsibilities:

Render board state
Display messages and errors

APIs:

void renderBoard(const Board *board);
void displayMessage(const char *msg);

Trigger:

Called after GameState update
Called when error occurs

3.5 Rules Agent

Headers:

rules.h

Responsibilities:

Validate moves
Enforce all game rules

APIs:

SelectionResult validateSelection(GameState *state, Position pos);
int isLegalMove(GameState *state, Move move);
int isCheck(GameState *state, Color player);
int isCheckmate(GameState *state, Color player);

Trigger:

Called before any move is applied

3.6 Move Generation Agent

Headers:

movegen.h

Responsibilities:

Generate all legal moves

APIs:

void generateMoves(GameState *state, MoveList *list);

Trigger:

Called during AI turn
Called during check/checkmate evaluation

3.7 Move System Agent

Headers:

move.h
move_list.h

Responsibilities:

Represent moves
Apply moves
Undo moves via history

APIs:

void applyMove(GameState *state, Move move);
void undoMove(GameState *state);

Trigger:

applyMove: after move validated
undoMove: when undo requested

3.8 GameState Agent

Headers:

gamestate.h

Responsibilities:

Store complete runtime state

3.9 Board Agent

Headers:

board.h
piece.h
position.h

Responsibilities:

Low-level board manipulation

3.10 AI Agent

Headers:

ai.h

Responsibilities:

Select optimal move

APIs:

Move selectBestMove(MoveList *moves, GameState *state);

Trigger:

Called when current player is AI
Called after turn switch

Behavior:

Generate moves
Select best move
Emit AI_MOVE_EVENT

3.11 Log Agent

Headers:

log.h

Responsibilities:

Record gameplay in human-readable format
Maintain consistency with move history

APIs:

int initLog(const GameConfig *config);
int logGameStart(const GameConfig *config);
int logMove(const GameState *state, Move move);
int rebuildLogFromHistory(const GameState *state);
int logGameEnd(const GameState *state);

Trigger:

logGameStart: when game begins
logMove: after each successful move (player or AI)
rebuildLogFromHistory: after undo or load
logGameEnd: when game terminates

Important Rule:
Undo operations are NOT logged.
Log is always reconstructed from MoveHistory.

Execution Protocol

The system executes the following loop:

while game is running:

Event event = pollEvent()

if event.type == INPUT_EVENT:
    translate input into gameplay or control event

if event.type == GAMEPLAY_EVENT:

    if move is invalid:
        handleNonFatalError()
    else:
        applyMove()
        logMove()

if event.type == AI_MOVE_EVENT:
    applyMove()
    logMove()

if event.type == CONTROL_EVENT:
    handle menu / undo / exit

    if undo triggered:
        undoMove()
        rebuildLogFromHistory()

if event.type == SYSTEM_EVENT:
    handle timer / game over detection

updateGameState()

renderBoard()
Error Handling Policy

Errors are categorized into:

Non-Fatal Errors
Fatal Errors

5.1 Non-Fatal Errors

Examples:

Invalid input
Illegal move
Selecting opponent piece
Empty selection

Behavior:

Reject the action
Do NOT modify GameState
Generate ERROR_EVENT
Display message via UI
Continue game loop

Implementation:

void handleNonFatalError(ErrorCode code):
displayMessage(error message)

5.2 Fatal Errors

Examples:

System failure
Corrupted GameState
Unexpected runtime error

Behavior:

Immediately terminate gameplay loop
Transition to GAME_TERMINATION_STATE
Skip End Game Menu
Display Fatal Error Window
After confirmation, return to MAIN_MENU_STATE

Implementation:

void handleFatalError(ErrorCode code):
transitionState(GAME_TERMINATION_STATE)
displayMessage("Fatal Error")

Event Flow

User Input → Command → Event → Controller → Rules → GameState → UI

Dependency Rules

Allowed:

UI → Input → Event → Controller → Rules → Board
Controller → AI
Controller → Log

Forbidden:

UI → Rules
AI → direct GameState mutation
Multiple modules modifying Board independently

Human-Readable Log Specification

8.1 Standard Format

[Move NN] HH:MM:SS | Player | Piece Origin -> Destination

Example:

[Move 01] 00:00:05 | White | Pawn F2 -> F4
[Move 02] 00:00:12 | Black (AI) | Pawn E7 -> E5

8.2 Capture

| Capture: Player Piece

8.3 Special Move

| Special: Description

8.4 Anteater Multi-Capture

[Move NN] ... | Anteater D6 -> G6 | Capture Chain: E6, F6, G6 | Capture Count: 3 | Special: Ant Eating

8.5 Check and Checkmate

| Result: Check
| Result: Checkmate

8.6 Game Start and End

[Game Start] ...
[Game End] ...

Undo Consistency Guarantee

Undo is NOT recorded in the log.

Instead:

MoveHistory is the authoritative source
Log is derived from MoveHistory

After undo:

Remove last move from MoveHistory
Restore GameState
Rebuild log file from MoveHistory

Rebuild Algorithm:

clearLogFile()

logGameStart()

for each move in moveHistory:
logMove(move)

if game ended:
logGameEnd()

Invariant:

Log equals serialized MoveHistory

Final Statement:

The log is not incrementally modified during undo.
It is reconstructed from move history to ensure consistency.
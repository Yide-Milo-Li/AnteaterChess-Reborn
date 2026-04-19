agent.md — Anteater Chess Agent Specification
1. Overview

This document defines the system agents, their responsibilities, and their mappings to header files in the Anteater Chess software architecture.

The system follows a modular, event-driven architecture in which all components interact through well-defined APIs and a centralized GameState.

2. Core Architecture Principles
Single Source of Truth
GameState is the only authoritative runtime state.
Event-Driven Execution
All interactions are translated into Events and processed by the FSM.
Strict Module Responsibility
Each module (header file) owns a single responsibility.
Layered Design
UI → Event → Controller → Rules → Board/Data
3. Agent Mapping to Header Files

This section defines how each header file corresponds to a system agent.

3.1 Controller Agent

Header Files:

fsm.h
controller.h

Responsibilities:

Main game loop
Event dispatching
State transitions
Coordination of all modules

Key APIs:

void runGameLoop(GameState *state);
Event pollEvent();
void processEvent(GameState *state, Event event);
void transitionState(GameState *state, SystemState newState);

Notes:

This is the core orchestrator of the entire system
No business logic should exist outside this control flow
3.2 Event System Agent

Header Files:

event.h
event_queue.h

Responsibilities:

Unified representation of all system inputs
Event buffering and prioritization
Decoupling input sources from logic

Key APIs:

int enqueueEvent(EventQueue *q, Event e, QueueType type);
Event dequeueGameplayEvent(EventQueue *q);
Event dequeueSystemEvent(EventQueue *q);
Event dequeueControlEvent(EventQueue *q);

Notes:

All modules must communicate through Events
No direct cross-module triggering
3.3 Input Agent

Header Files:

input.h

Responsibilities:

Capture raw user input
Translate input into Commands
Provide input abstraction layer

Key APIs:

int getMoveInput(Command *cmd);
Position getBoardClick();
int getBoardInput(Position *pos, InputType *type);

Notes:

Does NOT modify GameState
Only produces Commands → converted into Events
3.4 UI Agent

Header Files:

ui.h
board_renderer.h (if separated)

Responsibilities:

Display board
Display game state
Provide visual feedback

Key APIs:

void renderBoard(const Board *board);
void displayMessage(const char *msg);

Notes:

UI is read-only relative to GameState
No game logic allowed
3.5 Rules Engine Agent

Header Files:

rules.h

Responsibilities:

Enforce all chess + anteater rules
Validate moves and selections
Detect check and checkmate

Key APIs:

SelectionResult validateSelection(GameState *state, Position pos);
int isLegalMove(GameState *state, Move move);
int isCheck(GameState *state, Color player);
int isCheckmate(GameState *state, Color player);

Notes:

Pure logic module
No side effects (should not mutate state directly)
3.6 Move Generation Agent

Header Files:

movegen.h (or equivalent)

Responsibilities:

Generate possible moves
Provide candidate move lists

Key APIs:

void generateMoves(GameState *state, MoveList *list);
void generatePieceMoves(Board *board, Position pos, MoveList *list);

Notes:

Works closely with rules.h
Does NOT decide best move
3.7 Move System Agent

Header Files:

move.h
move_list.h

Responsibilities:

Represent moves
Store move history
Support undo functionality

Key APIs:

void applyMove(GameState *state, Move move);
void undoMove(GameState *state);
void addMoveToList(MoveList *list, Move move);

Notes:

Must support special moves (castling, en passant, anteater capture)
Must support multi-capture chains
3.8 GameState Agent

Header Files:

gamestate.h

Responsibilities:

Store all runtime data
Provide unified access to system state

Structure:

typedef struct {
Board board;
Player players[2];

int currentTurn;
int moveCount;
int gameOver;

SystemState systemState;

} GameState;

Notes:

Passed to all modules
No module should duplicate this data
3.9 Board / Piece Agent

Header Files:

board.h
piece.h
position.h

Responsibilities:

Represent core game data structures
Provide low-level board operations

Key APIs:

Piece getPieceAt(Board *board, Position pos);
void setPieceAt(Board *board, Position pos, Piece piece);
void movePiece(Board *board, Position from, Position to);

Notes:

No rule checking here
Pure data manipulation only
3.10 AI Agent

Header Files:

ai.h

Responsibilities:

Select best move from legal moves
Implement strategy

Key APIs:

Move selectBestMove(MoveList *moves, GameState *state);

Notes:

Must not modify GameState directly
Only returns a Move
3.11 Log Agent

Header Files:

log.h

Responsibilities:

Record move history
Output human-readable logs

Key APIs:

void logMove(const Move *move);
void saveLogToFile(const char *filename);

Notes:

Required feature
Must support readable format
3.12 Timer Agent (Optional / Advanced)

Header Files:

timer.h
turn_timer.h

Responsibilities:

Track player time
Trigger timeout events

Key APIs:

void startTimer(Player *player);
void updateTimer(GameState *state);
int isTimeUp(Player *player);

Notes:

Should integrate with Event system
4. Event Flow Model

User Input → Command → Event → FSM → GameState Update → UI Render

Important Rules:

UI cannot modify GameState
Only Controller + Rules can modify GameState
AI produces Move, not state changes
5. Dependency Rules

Allowed:

UI → Input → Event → Controller → Rules → Board

Controller → AI
Controller → Log

Forbidden:

UI → Rules
AI → GameState mutation
Multiple modules writing Board independently

6. Feature Responsibility Mapping

Move Input → Input + Event
Move Validation → Rules
Move Execution → Controller + Move System
AI Move → AI
Undo → Move System
Logging → Log
Timer → Timer Module
Hint → AI

7. Development Guidelines
Each header file defines one module (one agent)
APIs must be stable before implementation
Modules should be independently testable
Avoid circular dependencies
8. Summary

This system is:

Event-driven
Modular
Data-centric
Deterministic

Each header file represents a clear agent with:

Defined responsibility
Defined API
Controlled interaction boundaries
#include <avr/io.h>
#include <avr/interrupt.h>
#include "chess.h"
#include "lcd.h"
#include "timer.h"
#include "switch.h"
#include "led.h"
#include "shiftregister.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

enum class State : uint8_t {inProgress, illegalMove, waitGame, debouncePress, waitRelease, debounceRelease};

volatile State currState = State::waitGame;
volatile bool scan = false;

static void printMessage(const char* line1, const char* line2) {
    moveCursor(0, 0);
    writeString("        ");
    moveCursor(0, 0);
    writeString(line1);
    moveCursor(0, 1);
    writeString("        ");
    moveCursor(0, 1);
    writeString(line2);

}

// find the index of the only 1 bit in a binary number
// used to construct the to and from move
static uint8_t findBitIndex(uint64_t num) {
    uint8_t index = 0;
    while ((num & 1) == 0) {
        num >>= 1;
        index++;
        if (index > 63) {
            return 64;
        }
    }
    return index;
}

// Finds the correct index of the square moved from, loops over all squares, after the
// first piece is found a king or correct colored piece can still override
// since castling moves two pieces and a rook would be found first for queenside castles.
// The wrong color logic is here so that tryMove can report illegal moves and the board doesn't
// get stuck in an endless cycle
static bool findFrom(Position& position, uint64_t stillEmpty, uint8_t& from) {
    bool found = false, foundOther = false;
    uint8_t other = 0;
    Piece piece;
    for (uint8_t sq = 0; sq < 64; sq++) {
        if (!(stillEmpty & (1ULL << sq))) continue;
        piece = position.board[sq];
        if (colorOf(piece) != position.turn) {
            if (!foundOther) {
                foundOther = true; other = sq;
            }
            continue;
        }
        if (!found || piece == Piece::whiteKing || piece == Piece::blackKing) {
            found = true;
            from = sq;
        }
    }
    if (!found && foundOther) {
        from = other;
        return true;
    }
    return found;
}

static bool findTo(uint64_t placed, uint64_t captured, uint8_t& to, uint8_t from) {
    if (placed) {
        // If there is exactly one new square occupied that is the move to square
        if (__builtin_popcountll(placed) == 1) {
            to = findBitIndex(placed);
            return true;
        }
        // Castling moves two pieces new squares, find the kings square
        for (uint8_t sq = 0; sq < 64; sq++) {
            if (placed & (1ULL << sq) && ((sq == from +2) || (sq == from -2))) {
                to = sq;
                return true;
            }
        }
        return false;
    }
    // If there is one captured square that is the move to square
    if (__builtin_popcountll(captured) == 1){
        to = findBitIndex(captured);
        return true;
    }
    return false;

}


int main () {
    Position currPos;
    currPos.fromFEN(START_FEN);
    Position tryPos;
    Move move;
    MoveType moveResult;
    uint64_t emptied = 0, expected = 0, currBoard = 0, prevBoard = 0, stillEmpty = 0, captured = 0, placed = 0;
    uint8_t stableCount = 0, capturedIndex = 0;


    // Initialize peripherals
    initTimer0();
    initTimer1();
    initTimer2();
    initLED();
    initLCD();
    initSwitchPE4();
    initSPI();
    sei();

    printMessage("Setup", "board");
    turnOnRedLED();
    // game loop
    while (1) {
        switch (currState) {
            case State::inProgress: {
                expected = currPos.occupancy();
                if (scan) {
                    prevBoard = currBoard;
                    scan = false;
                    currBoard = readBoard();
                    emptied |= expected & ~currBoard;
                    if (currBoard == prevBoard) stableCount++;
                    else stableCount = 0;
                }
                else break;

                // Wait for board to settle for ~400 ms
                if (stableCount == 40) {
                    stableCount = 0;
                    tryPos = currPos;
                    stillEmpty = expected & ~currBoard;
                    placed = currBoard & ~expected;
                    captured = currBoard & expected & emptied; // The last sqaure to be emptied that now has a piece on it was captured

                    // Clear squares that are the same color as the current turn. This is important in if a player picks a piece up, and then
                    // quickly captures a different piece. There would be two bits set in captured which would mess up the movement logic
                    for (uint8_t sq = 0; sq < 64; sq++) {
                        if ((captured & (1ULL << sq)) && colorOf(currPos.board[sq]) == currPos.turn)
                            captured &= ~(1ULL << sq);
                    }

                    // No move made
                    if (currBoard == expected) {
                        emptied = 0;
                        break;
                    }

                    bool foundFrom = findFrom(currPos, stillEmpty, move.from);
                    bool foundTo = findTo(placed, captured, move.to, move.from);

                    // A move was not made this cycle
                    if (!foundFrom || !foundTo) break;

                    moveResult = tryPos.tryMove(move);

                    if (moveResult == MoveType::legal) {
                        // Wait for the rook to be placed back
                        if (currBoard == tryPos.occupancy()) {
                            currPos = tryPos;
                            emptied  = 0;
                        }
                    }
                    else {
                        switch (moveResult) {
                            case MoveType::checkmate:
                                printMessage("Checkmt", "");
                                currPos.fromFEN(START_FEN);
                                delayMs(5000);
                                printMessage("Setup", "board");
                                turnOnRedLED():
                                currState = State::waitGame;
                                break;
                            case MoveType::stalemate:
                                printMessage("Stalemt", "");
                                currPos.fromFEN(START_FEN);
                                delayMs(5000);
                                printMessage("Setup", "board");
                                turnOnRedLED();
                                currState = State::waitGame;
                                break;
                            case MoveType::blockedPath:
                                printMessage("Blocked", "path");
                                turnOnRedLED();
                                currState = State::illegalMove;
                                break;
                            case MoveType::leavesKingInCheck:
                                printMessage("King in", "check");
                                turnOnRedLED();
                                currState = State::illegalMove;
                                break;
                            case MoveType::illegalCastle:
                                printMessage("Illegal", "castle");
                                turnOnRedLED();
                                currState = State::illegalMove;
                                break;
                            case MoveType::wrongTurn:
                                printMessage("Wrong", "turn");
                                turnOnRedLED();
                                currState = State::illegalMove;
                                break;
                            case MoveType::wrongMovement:
                                printMessage("Wrong", "Movement");
                                turnOnRedLED();
                                currState = State::illegalMove;
                                break;
                            case MoveType::selfCapture:
                                printMessage("Self", "capture");
                                turnOnRedLED();
                                currState = State::illegalMove;
                                break;
                        }
                        break;
                    }
                }
                break;
            }
            case State::debouncePress:
                delayMs(5);
                currState = State::waitRelease;
                break;
            case State::waitRelease:
                break;
            case State::debounceRelease:
                delayMs(5);
                printMessage("Setup", "board");
                currPos.fromFEN(START_FEN);
                turnOnRedLED();
                currState = State::waitGame;
                break;
            case State::illegalMove:
                if (scan) {
                    // Reset flags and trackers
                    scan = false;
                    stableCount = 0;
                    emptied = 0;
                    currBoard = readBoard();
                    if (currBoard == currPos.occupancy()) {
                        printMessage("In", "progress");
                        turnOnGreenLED();
                        currState = State::inProgress;
                    }
                }
                break;
            case State::waitGame:
                if (scan) {
                    // Reset flags and trackers
                    scan = false;
                    stableCount = 0;
                    emptied = 0;
                    currBoard = readBoard();
                    if (currBoard == currPos.occupancy()) {
                        printMessage("In", "progress");
                        turnOnGreenLED();
                        currState = State::inProgress;
                    }
                }
                break;

        }


    }

}

ISR(TIMER0_COMPA_vect){
    scan = true;
}

// When the button is pressed once wait unitl pieces are reset by players then
// start a new game
ISR(INT4_vect){
    switch (currState) {
        case State::illegalMove:
            currState = State::debouncePress;
            break;
        case State::inProgress:
            currState = State::debouncePress;
            break;
        case State::waitRelease:
            currState = State::debounceRelease;
    }
}

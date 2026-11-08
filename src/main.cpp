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

volatile State currState = State::inProgress;
volatile bool scan = false;

static void printMessage(char* line1, char* line2) {
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
static int findBitIndex(uint64_t num) {
    int index = 0;
    while ((num & 1) == 0) {
        num >>= 1;
        index++;
        if (index > 63) {
            return 64;
        }
    }
    return index;
}

// Finds the correct index of the square moved from, loops over all squares, only
// excepts pieces of the correct color, after the first piece is found a king can still override
// since castling moves two pieces and a rook would be found first for queenside castles
static bool findFrom(Position& position, uint64_t stillEmpty, uint8_t& from) {
    bool found = false;
    Piece piece;
    for (uint8_t sq = 0; sq < 64; sq++) {
        if (!(stillEmpty & (1ULL << sq))) continue;
        piece = position.board[sq];
        if (!found || piece == Piece::whiteKing || piece == Piece::blackKing) {
            found = true;
            from = sq;
        }
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
    uint8_t stableCount = 0;


    // Initialize peripherals
    initTimer0();
    initTimer1();
    initTimer2();
    initLED();
    initLCD();
    initSwitchPE4();
    initSPI();
    sei();

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
                                currState = State::waitGame;
                                break;
                            case MoveType::stalemate:
                                printMessage("Stalemt", "");
                                currState = State::waitGame;
                                break;
                            case MoveType::blockedPath:
                                printMessage("Blocked", "path");
                                currState = State::illegalMove;
                                break;
                            case MoveType::leavesKingInCheck:
                                printMessage("King in", "check");
                                currState = State::illegalMove;
                                break;
                            case MoveType::illegalCastle:
                                printMessage("Illegal", "castle");
                                currState = State::illegalMove;
                                break;
                            case MoveType::wrongTurn:
                                printMessage("Wrong", "turn");
                                currState = State::illegalMove;
                                break;
                            case MoveType::wrongMovement:
                                printMessage("Wrong", "Movement");
                                currState = State::illegalMove;
                                break;
                            case MoveType::selfCapture:
                                printMessage("Self", "capture");
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
                        currState = State::inProgress;
                    }
                }
                break;
            case State::waitGame:
                currPos.fromFEN(START_FEN);
                if (scan) {
                    // Reset flags and trackers
                    scan = false;
                    stableCount = 0;
                    emptied = 0;
                    currBoard = readBoard();
                    if (currBoard == currPos.occupancy()) {
                        printMessage("In", "progress");
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

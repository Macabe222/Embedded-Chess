#ifndef CHESS_H
#define CHESS_H

#include <stdint.h>

// Board indexing: board[0] = a8 (top left)

enum class Color : uint8_t {white, black, empty};

enum class Piece : uint8_t {
    empty, whiteKing, whiteQueen, whitePawn, whiteBishop, whiteKnight, whiteRook,
    blackKing, blackQueen, blackPawn, blackBishop, blackKnight, blackRook
};

enum class MoveType : uint8_t {
    legal, blockedPath, leavesKingInCheck, illegalCastle, wrongTurn, wrongMovement, selfCapture, checkmate,
    stalemate
};

struct Move {
    uint8_t from;
    uint8_t to;
};

class Position {
public:
    Piece board[64];
    Color turn;

    // Four bits: bit 0 white kingside, 1 white queenside, 2 black kingside, 3 black queenside
    uint8_t castleRights;
    int8_t enPassantSquare;     // -1 if none
    uint8_t halfMove;
    uint16_t fullMove;

    // Load a position from a FEN string. Returns false (and leaves the
    // position unchanged) if the string is malformed.
    bool fromFEN(const char* fen);


    // Make a move without checking legality (castling, en passant, promotion
    // and all state updates included). Use tryMove for player moves.
    void applyMove(Move& move);

    // Check a move and make it only if it is legal. On any other result the
    // position is left unchanged.
    MoveType tryMove(Move& move);

    // Logic to be called by try move when checking stalemate/checkmate
    MoveType tryMoveInner(Move& move);

    // One bit per occupied square, bit 0 = a8, same numbering as board[]
    uint64_t occupancy();

#ifndef __AVR__
    // Debug printing for PC builds only
    void print();
#endif
};

// Helpers used by tryMove (also handy for testing individual rules)
Color colorOf(Piece piece);
uint8_t findKingSquare(Position& position, Color side);
bool inCheck(Position& position, Color side);
bool validPieceMovement(Position& position, Move& move);
bool openPath(Position& position, Move& move);
bool isCastle(Position& position, Move& move);
bool validCastle(Position& position, Move& move);

#endif // CHESS_H

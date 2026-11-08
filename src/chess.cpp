#include "chess.h"

#ifndef __AVR__
#include <stdio.h>
#endif

static Piece charToPiece(char c) {
    switch (c) {
        case 'P': return Piece::whitePawn;
        case 'N': return Piece::whiteKnight;
        case 'B': return Piece::whiteBishop;
        case 'R': return Piece::whiteRook;
        case 'Q': return Piece::whiteQueen;
        case 'K': return Piece::whiteKing;
        case 'p': return Piece::blackPawn;
        case 'n': return Piece::blackKnight;
        case 'b': return Piece::blackBishop;
        case 'r': return Piece::blackRook;
        case 'q': return Piece::blackQueen;
        case 'k': return Piece::blackKing;
        default: return Piece::empty;
    }
}

// Parse more than one digit numbers for half and full move
static bool parseUInt(const char*& p, uint16_t& out) {
    if (*p < '0' || *p > '9') return false;
    out = 0;
    while (*p >= '0' && *p <= '9') {
        out = out * 10 + (*p - '0');
        p++;
    }
    return true;
}

bool Position::fromFEN(const char* fen) {
    // Make a temp position bc we only want to update the position on a valid board position
    Position temp;
    const char* p = fen;
    int row = 0, file = 0;   // FEN starts at a8, which is now board[0]

    for (auto& sq : temp.board) sq = Piece::empty;

    // Traverse FEN string
    while (*p && *p != ' ') {
        char c = *p++;
        if (c == '/') {
            if (file != 8 || row == 7) return false;
            row++;           // next rank down
            file = 0;
        }
        else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) return false;
        }
        else {
            Piece piece = charToPiece(c);
            if (piece == Piece::empty || file > 7) return false;
            temp.board[row * 8 + file] = piece;
            file++;
        }
    }

    // Make sure board is evenly filled
    if (row != 7 || file != 8) return false;
    if (*p++ != ' ') return false;

    // Parse side to move
    if (*p == 'w') temp.turn = Color::white;
    else if (*p == 'b') temp.turn = Color::black;
    else return false;
    p++;
    if (*p++ != ' ') return false;

    // Parse castling rights
    temp.castleRights = 0;
    if (*p == '-') {
        p++;
    } else {
        while (*p && *p != ' ') {
            switch (*p) {
                case 'K': temp.castleRights |= 0b0001; break;
                case 'Q': temp.castleRights |= 0b0010; break;
                case 'k': temp.castleRights |= 0b0100; break;
                case 'q': temp.castleRights |= 0b1000; break;
                default: return false;
            }
            p++;
        }
    }
    if (*p++ != ' ') return false;

    // En passant: rank '8' is row 0, rank '1' is row 7
    if (*p == '-')  {
        temp.enPassantSquare = -1;
        p++;
    }
    else {
        char f = p[0], r = p[1];
        if (f < 'a' || f > 'h' || (r != '3' && r != '6'))  return false;
        temp.enPassantSquare = ('8' - r) * 8 + (f - 'a');
        p += 2;
    }

    // halfmove and fullmove
    temp.halfMove = 0;
    temp.fullMove = 1;
    if (*p == ' ') {
        p++;
        uint16_t n;
        if (!parseUInt(p, n) || n > 255) return false;
        temp.halfMove = n;
        if (*p == ' ') {
            p++;
            if (!parseUInt(p, n) || n == 0) return false;
            temp.fullMove = n;
        }
    }

    *this = temp;
    return true;
}

#ifndef __AVR__
void Position::print(){
    static const char chars[] = ".KQPBNRkqpbnr";

    // Row 0 (rank 8) is printed first, at the top
    for (int row = 0; row < 8; row++) {
        printf("%d ", 8 - row);
        for (int file = 0; file < 8; file++) {
            printf("%c ", chars[static_cast<int>(board[row * 8 + file])]);
        }
        printf("\n");
    }
    printf("  a b c d e f g h\n\n");

    printf("Turn:       %s\n", turn == Color::white ? "White" : "Black");

    printf("Castling:   ");
    if (castleRights == 0) {
        printf("-");
    } else {
        if (castleRights & 1) printf("K");
        if (castleRights & 2) printf("Q");
        if (castleRights & 4) printf("k");
        if (castleRights & 8) printf("q");
    }
    printf("\n");

    printf("En passant: ");
    if (enPassantSquare < 0) {
        printf("-\n");
    } else {
        printf("%c%c\n", 'a' + enPassantSquare % 8, '8' - enPassantSquare / 8);
    }
    printf("Halfmove:   %d\nFullmove:   %d\n", halfMove, fullMove);
}
#endif

void Position::applyMove(Move& move) {
    Piece movePiece    = board[move.from];
    Piece capturePiece = board[move.to];
    bool isPawn = (movePiece == Piece::whitePawn || movePiece == Piece::blackPawn);
    bool isKing = (movePiece == Piece::whiteKing || movePiece == Piece::blackKing);
    int8_t diff = move.to - move.from;

    bool isCastle    = isKing && (diff == 2 || diff == -2);
    bool isEnPassant = isPawn && move.to == enPassantSquare;
    bool isPromote   = isPawn && (move.to < 8 || move.to >= 56);   // row 0 or row 7

    // Castle rights
    if (movePiece == Piece::whiteKing) castleRights &= ~0b0011;
    if (movePiece == Piece::blackKing) castleRights &= ~0b1100;
    if (move.from == 63 || move.to == 63) castleRights &= ~0b0001;  // h1: white kingside
    if (move.from == 56 || move.to == 56) castleRights &= ~0b0010;  // a1: white queenside
    if (move.from == 7  || move.to == 7)  castleRights &= ~0b0100;  // h8: black kingside
    if (move.from == 0  || move.to == 0)  castleRights &= ~0b1000;  // a8: black queenside

    board[move.to]   = movePiece;
    board[move.from] = Piece::empty;

    // Autoqueen (maybe change later)
    if (isPromote) {
        board[move.to] = (turn == Color::white) ? Piece::whiteQueen : Piece::blackQueen;
    }
    else if (isCastle) {
        if (diff == 2) {   // kingside
            board[move.from + 1] = board[move.from + 3];
            board[move.from + 3] = Piece::empty;
        } else {           // queenside
            board[move.from - 1] = board[move.from - 4];
            board[move.from - 4] = Piece::empty;
        }
    }
    else if (isEnPassant) {
        if (turn == Color::white) {
            board[move.to + 8] = Piece::empty;
        }
        else {
            board[move.to - 8] = Piece::empty;
        }
    }

    // En passant square: set after a double pawn push, cleared otherwise
    if (isPawn && (diff == 16 || diff == -16)) enPassantSquare = (move.from + move.to) / 2;
    else enPassantSquare = -1;

    // Counters and turn
    if (isPawn || capturePiece != Piece::empty) halfMove = 0;
    else halfMove++;
    if (turn == Color::black) fullMove++;
    turn = (turn == Color::white) ? Color::black : Color::white;
}

uint8_t findKingSquare(Position& position, Color side) {
    Piece targetKing = (side == Color::white) ? Piece::whiteKing : Piece::blackKing;

    for (uint8_t i = 0; i < 64; i++) {
        if (position.board[i] == targetKing) return i;
    }

    return 0;
}

// Given a position return true if in check
bool inCheck(Position& position, Color side) {
    Piece enemyRook  = (side == Color::white) ? Piece::blackRook  : Piece::whiteRook;
    Piece enemyQueen = (side == Color::white) ? Piece::blackQueen : Piece::whiteQueen;
    Piece enemyBishop = (side == Color::white) ? Piece::blackBishop : Piece::whiteBishop;
    Piece enemyPawn = (side == Color::white) ? Piece::blackPawn : Piece::whitePawn;
    Piece enemyKnight = (side == Color::white) ? Piece::blackKnight : Piece::whiteKnight;
    Piece enemyKing = (side == Color::white) ? Piece::blackKing : Piece::whiteKing;

    // Rank: right first, then left
    uint8_t kingSquare = findKingSquare(position, side);
    uint8_t square = kingSquare;
    int8_t direction = 1;
    while (true) {
        bool atEdge = (direction == 1) ? (square % 8 == 7) : (square % 8 == 0);
        if (!atEdge) {
            square += direction;
            Piece piece = position.board[square];
            if (piece == enemyRook || piece == enemyQueen) return true;
            if (piece == Piece::empty) continue;   // keep going this way
        }
        // Reached the edge or hit a blocking piece: switch sides or stop
        if (direction == 1) {
            direction = -1;
            square = kingSquare;
        } else {
            break;
        }
    }

    // File: down first (+8), then up (-8)
    square = kingSquare;
    direction = 8;
    while (true) {
        bool atEdge = (direction == 8) ? (square >= 56) : (square < 8);
        if (!atEdge) {
            square += direction;
            Piece piece = position.board[square];
            if (piece == enemyRook || piece == enemyQueen) return true;
            if (piece == Piece::empty) continue;
        }
        if (direction == 8) {
            direction = -8;
            square = kingSquare;
        } else {
            break;
        }
    }

    // Check queen/bishop on same diagonal
    square = kingSquare;
    direction = -7; // up right
    while (true) {
        bool atEdge = (direction == -7) ? (square % 8 == 7 || square < 8) : (square % 8 == 0 || square >= 56);
        if (!atEdge) {
            square += direction;
            Piece piece = position.board[square];
            if (piece == enemyBishop || piece == enemyQueen) return true;
            if (piece == Piece::empty) continue;
        }
        if (direction == -7) {
            direction = 7;
            square = kingSquare;
        }
        else {
            break;
        }
    }
    square = kingSquare;
    direction = -9;
    while (true) {
        bool atEdge = (direction == 9) ? (square % 8 == 7 || square >= 56) : (square % 8 == 0 || square < 8);
        if (!atEdge) {
            square += direction;
            Piece piece = position.board[square];
            if (piece == enemyBishop || piece == enemyQueen) return true;
            if (piece == Piece::empty) continue;
        }
        if (direction == -9) {
            direction = 9;
            square = kingSquare;
        }
        else {
            break;
        }
    }
    // Check pawn on checking square
    uint8_t kingFile = kingSquare % 8;
    if (side == Color::white) {
        if (kingSquare >= 8) {
            if (kingFile != 0 && position.board[kingSquare - 9] == enemyPawn) return true;  // up-left
            if (kingFile != 7 && position.board[kingSquare - 7] == enemyPawn) return true;  // up-right
        }
    } else {
        if (kingSquare < 56) {
            if (kingFile != 0 && position.board[kingSquare + 7] == enemyPawn) return true;  // down-left
            if (kingFile != 7 && position.board[kingSquare + 9] == enemyPawn) return true;  // down-right
        }
    }
    // Check knight in checking square
    uint8_t kingRank = kingSquare / 8;

    // all possible steps away from a king where a knight would be giving a check [row, file]
    static const int8_t knightSteps[8][2] = {
        {-2,-1}, {-2,1}, {-1,-2}, {-1,2}, {1,-2}, {1,2}, {2,-1}, {2,1}
    };

    for (auto& s : knightSteps) {
        int8_t row = kingRank + s[0];
        int8_t file = kingFile + s[1];
        if (row >= 0 && row < 8 && file >= 0 && file < 8) {
            if (position.board[row * 8 + file] == enemyKnight) return true;
        }
    }
    // Check king in checking square
    // cant give a check with the king ofc but this makes sure you cant move your king towards the enemy king
    for (int downRow = -1; downRow <= 1; downRow++) {
        for (int downFile = -1;  downFile <= 1; downFile++){
            if (downRow == 0 && downFile == 0) continue;
            uint8_t row = downRow + kingRank;
            uint8_t file = downFile + kingFile;
            if (row < 0 || row > 7 || file < 0 || file > 7) continue; // out of bounds
            if (position.board[row * 8 + file ] == enemyKing) return true;
        }
    }

    return false;
}

bool openPath(Position& position, Move& move) {
    Piece movePiece = position.board[move.from];
    int8_t diff = move.to - move.from;
    uint8_t square = move.from;
    int8_t rowChange  = move.to / 8 - move.from / 8;
    int8_t fileChange = move.to % 8 - move.from % 8;
    int8_t rowStep  = (rowChange  > 0) - (rowChange  < 0);
    int8_t fileStep = (fileChange > 0) - (fileChange < 0);
    int8_t direction = rowStep * 8 + fileStep;

    // ignore knights, their path cant be blocked
    if (movePiece == Piece::whiteKnight || movePiece == Piece::blackKnight) {
        return true;
    }
    square += direction;
    while (square != move.to) {
        if (position.board[square] != Piece::empty) return false;
        square += direction;
    }

    return true;
}

bool validPieceMovement(Position& position, Move& move) {
    Piece movePiece = position.board[move.from];
    int8_t diff = move.to - move.from;
    int rowChange  = move.to / 8 - move.from / 8;
    int fileChange = move.to % 8 - move.from % 8;
    int absRow  = rowChange  < 0 ? -rowChange  : rowChange;
    int absFile = fileChange < 0 ? -fileChange : fileChange;

    switch (movePiece) {
        case Piece::whitePawn:
            switch (diff) {
                case -16:
                    // Can only move two from the starting row
                    return position.board[move.to] == Piece::empty && move.from / 8 == 6;
                case -8:
                    return (position.board[move.to] == Piece::empty);
                case -7:
                case -9:
                    if (absFile != 1) return false; // So you cant move from a to h file
                    return position.board[move.to] != Piece::empty || position.enPassantSquare == move.to;
                default:
                    return false;
            }
        case Piece::blackPawn:
            switch (diff) {
                case 16:
                    // Can only move two from the starting row
                    return position.board[move.to] == Piece::empty && move.from / 8 == 1;
                case 8:
                    return (position.board[move.to] == Piece::empty);
                case 7:
                case 9:
                    if (absFile != 1) return false; // So you cant move from a to h file
                    return position.board[move.to] != Piece::empty || position.enPassantSquare == move.to;
                default:
                    return false;
            }
        case Piece::whiteBishop:
        case Piece::blackBishop:
            return absRow == absFile && absRow != 0;
        case Piece::whiteKnight:
        case Piece::blackKnight:
            return (absRow == 2 && absFile == 1) || (absRow == 1 && absFile == 2);
        case Piece::whiteRook:
        case Piece::blackRook:
            return (absRow == 0 && absFile != 0) || (absRow != 0 && absFile == 0);
        case Piece::whiteQueen:
        case Piece::blackQueen:
            return (absRow == absFile && absRow != 0) || ((absRow == 0 && absFile != 0) || (absRow != 0 && absFile == 0));
        case Piece::whiteKing:
        case Piece::blackKing:
            return absRow <= 1 && absFile <= 1 && (absRow + absFile) != 0;
        default: return false;
    }
}

bool isCastle(Position& position, Move& move) {
    Piece piece = position.board[move.from];
    int8_t diff = move.to - move.from;
    return (piece == Piece::whiteKing || piece == Piece::blackKing) && (diff == 2 || diff == -2);
}

bool validCastle(Position& position, Move& move) {
    int8_t diff = move.to - move.from;
    Piece king = position.board[move.from];
    bool white = (king == Piece::whiteKing);

    // Check castling rights
    uint8_t right;
    if (white) right = (diff == 2) ? (1 << 0) : (1 << 1);
    else       right = (diff == 2) ? (1 << 2) : (1 << 3);
    if (!(position.castleRights & right)) return false;

    // Rook and king need to be on the start squares
    uint8_t kingStart = white ? 60 : 4;
    uint8_t rookSquare = (diff == 2) ? kingStart + 3 : kingStart - 4;
    Piece rook = white ? Piece::whiteRook : Piece::blackRook;
    if (move.from != kingStart || position.board[rookSquare] != rook) return false;

    // Make sure
    int step = (diff == 2) ? 1 : -1;
    for (int sq = kingStart + step; sq != rookSquare; sq += step) {
        if (position.board[sq] != Piece::empty) return false;
    }

    // Can't castle out of check
    Color side = white ? Color::white : Color::black;
    if (inCheck(position, side)) return false;

    // Can't castle through a check
    Position test = position;
    test.board[kingStart] = Piece::empty;
    test.board[kingStart + step] = king;
    if (inCheck(test, side)) return false;

    return true;
}

Color colorOf(Piece piece) {
    switch (piece) {
        case Piece::whiteKing: return Color::white; break;
        case Piece::whiteQueen: return Color::white; break;
        case Piece::whiteBishop: return Color::white; break;
        case Piece::whiteKnight: return Color::white; break;
        case Piece::whiteRook: return Color::white; break;
        case Piece::whitePawn: return Color::white; break;
        case Piece::blackKing: return Color::black; break;
        case Piece::blackQueen: return Color::black; break;
        case Piece::blackBishop: return Color::black; break;
        case Piece::blackKnight: return Color::black; break;
        case Piece::blackRook: return Color::black; break;
        case Piece::blackPawn: return Color::black; break;
        case Piece::empty: return Color::empty; break;
    }

    return Color::empty;
}

// Detects checkmate and stalemates by iterating all possible moves and if any
// of them are legal returning MoveType::legal, otherwise checks if the the king is
// in check, if it is return checkmate, otherwise return stalemate
MoveType isGameOver(Position& position) {
    Move move;
    MoveType moveResult;
    Position test = position;

    for (uint8_t from = 0; from < 64; from++) {
        if (colorOf(position.board[from]) != position.turn) continue;
        if (position.board[from] == Piece::empty) continue;
        for (uint8_t to = 0; to < 64; to++) {
            move.to = to;
            move.from = from;
            moveResult = test.tryMoveInner(move);
            if (moveResult == MoveType::legal) return moveResult;
        }
    }

    if (inCheck(position, position.turn)) return MoveType::checkmate;
    return MoveType::stalemate;

}

MoveType Position::tryMoveInner(Move& move) {
    Position tempBoard = *this;
    Piece movePiece = board[move.from];
    Piece capturePiece = board[move.to];

    if (colorOf(movePiece) != turn) return MoveType::wrongTurn;
    if (colorOf(movePiece) == colorOf(capturePiece)) return MoveType::selfCapture;
    if (isCastle(*this, move)) {
        if (!validCastle(*this, move)) return MoveType::illegalCastle;
    }
    else {
        if (!validPieceMovement(*this, move)) return MoveType::wrongMovement;
        if (!openPath(*this, move)) return MoveType::blockedPath;
    }


    tempBoard.applyMove(move);
    if (inCheck(tempBoard, turn)) return MoveType::leavesKingInCheck;


    *this = tempBoard;

    return MoveType::legal;

}

MoveType Position::tryMove(Move& move) {
    MoveType moveResult = tryMoveInner(move);
    if (moveResult != MoveType::legal) return moveResult;

    return isGameOver(*this);
}

// Convert board from position instance into 64 bits, each representing a square being occupied
// Bit 0 = a8 (top left indexing)
uint64_t Position::occupancy() {
    uint64_t bits = 0;
    for (int sq = 0; sq < 64; sq ++) {
        if (board[sq] != Piece::empty) {
            bits |= (1ULL << sq);
        }
    }
    return bits;
}

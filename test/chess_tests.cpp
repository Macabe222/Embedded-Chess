#include "chess.h"
#include <stdio.h>

// ===================== TESTS =====================

static int testsRun = 0, testsPassed = 0;

static void report(const char* label, bool pass, const char* expected, const char* got) {
    testsRun++;
    if (pass) testsPassed++;
    printf("%-46s %s", label, pass ? "PASS" : "FAIL");
    if (!pass) printf("   (expected %s, got %s)", expected, got);
    printf("\n");
}

// Square name -> index, so tests read like chess: sq("e2") == 52
static uint8_t sq(const char* name) {
    return ('8' - name[1]) * 8 + (name[0] - 'a');
}

static const char* typeName(MoveType t) {
    switch (t) {
        case MoveType::legal:             return "legal";
        case MoveType::blockedPath:       return "blockedPath";
        case MoveType::leavesKingInCheck: return "leavesKingInCheck";
        case MoveType::illegalCastle:     return "illegalCastle";
        case MoveType::wrongTurn:         return "wrongTurn";
        case MoveType::wrongMovement:     return "wrongMovement";
        case MoveType::selfCapture:       return "selfCapture";
    }
    return "?";
}

static bool load(Position& p, const char* label, const char* fen) {
    if (p.fromFEN(fen)) return true;
    report(label, false, "valid FEN", "parse failure");
    return false;
}

// fromFEN: should the string parse?
static void fenTest(const char* label, const char* fen, bool expected) {
    Position p;
    bool got = p.fromFEN(fen);
    report(label, got == expected, expected ? "true" : "false", got ? "true" : "false");
}

// applyMove: the resulting position must equal the position in fenAfter
// (board, turn, castling rights, en passant square, both counters)
static void applyTest(const char* label, const char* fenBefore, const char* from, const char* to,
                      const char* fenAfter) {
    Position p, expected;
    if (!load(p, label, fenBefore) || !load(expected, label, fenAfter)) return;
    Move m{sq(from), sq(to)};
    p.applyMove(m);

    const char* problem = nullptr;
    for (int i = 0; i < 64 && !problem; i++)
        if (p.board[i] != expected.board[i]) problem = "board differs";
    if (!problem && p.turn != expected.turn)                       problem = "turn differs";
    if (!problem && p.castleRights != expected.castleRights)       problem = "castling rights differ";
    if (!problem && p.enPassantSquare != expected.enPassantSquare) problem = "en passant square differs";
    if (!problem && p.halfMove != expected.halfMove)               problem = "halfmove differs";
    if (!problem && p.fullMove != expected.fullMove)               problem = "fullmove differs";

    report(label, problem == nullptr, "matching position", problem ? problem : "");
    if (problem) { printf("  got:\n"); p.print(); }
}

// inCheck
static void checkTest(const char* label, const char* fen, Color side, bool expected) {
    Position p;
    if (!load(p, label, fen)) return;
    bool got = inCheck(p, side);
    report(label, got == expected, expected ? "true" : "false", got ? "true" : "false");
}

// validPieceMovement: is the move the right shape for the piece?
static void shapeTest(const char* label, const char* fen, const char* from, const char* to, bool expected) {
    Position p;
    if (!load(p, label, fen)) return;
    Move m{sq(from), sq(to)};
    bool got = validPieceMovement(p, m);
    report(label, got == expected, expected ? "true" : "false", got ? "true" : "false");
}

// openPath: are the squares in between empty? (only for moves with a valid shape)
static void pathTest(const char* label, const char* fen, const char* from, const char* to, bool expected) {
    Position p;
    if (!load(p, label, fen)) return;
    Move m{sq(from), sq(to)};
    bool got = openPath(p, m);
    report(label, got == expected, expected ? "true" : "false", got ? "true" : "false");
}

// tryMove: full legality result
static void moveTest(const char* label, const char* fen, const char* from, const char* to, MoveType expected) {
    Position p;
    if (!load(p, label, fen)) return;
    Move m{sq(from), sq(to)};
    MoveType got = p.tryMove(m);
    report(label, got == expected, typeName(expected), typeName(got));
}

// tryMove must leave the position untouched when it rejects a move
static void unchangedOnRejectTest(const char* label, const char* fen, const char* from, const char* to) {
    Position p, before;
    if (!load(p, label, fen) || !load(before, label, fen)) return;
    Move m{sq(from), sq(to)};
    MoveType got = p.tryMove(m);
    bool same = true;
    for (int i = 0; i < 64; i++) if (p.board[i] != before.board[i]) same = false;
    if (p.turn != before.turn) same = false;
    report(label, got != MoveType::legal && same, "rejected, position unchanged",
           got == MoveType::legal ? "move accepted" : "position modified");
}

int main () {
    const char* START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    printf("--- fromFEN ---\n");
    fenTest("Starting position parses",              START, true);
    fenTest("Mid-game FEN parses",                   "6k1/pp2R3/2p1N1pp/8/8/8/PP4PP/7K b - - 2 33", true);
    fenTest("FEN without counters parses",           "4k3/8/8/8/8/8/8/4K3 w - -", true);
    fenTest("Rank with 9 squares fails",             "rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false);
    fenTest("Only 7 ranks fails",                    "rnbqkbnr/pppppppp/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false);
    fenTest("Bad side to move fails",                "4k3/8/8/8/8/8/8/4K3 x - - 0 1", false);
    fenTest("Bad castling letter fails",             "4k3/8/8/8/8/8/8/4K3 w X - 0 1", false);
    fenTest("En passant on rank 4 fails",            "4k3/8/8/8/8/8/8/4K3 w - e4 0 1", false);
    fenTest("Unknown piece letter fails",            "4k3/8/8/8/8/8/8/4X3 w - - 0 1", false);
    {
        Position p;
        p.fromFEN("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
        report("Square/field values after parse",
               p.board[sq("a8")] == Piece::blackRook && p.board[sq("e1")] == Piece::whiteKing &&
               p.board[sq("e4")] == Piece::whitePawn && p.board[sq("e2")] == Piece::empty &&
               p.turn == Color::black && p.castleRights == 0b1111 &&
               p.enPassantSquare == sq("e3"),
               "a8=r, e1=K, e4=P, e2 empty, black, KQkq, ep e3", "different values");
    }

    printf("\n--- applyMove ---\n");
    applyTest("e2-e4 sets en passant, resets clock",  START, "e2", "e4",
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    applyTest("Black reply increments fullmove",      "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", "e7", "e5",
              "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    applyTest("Knight move increments halfmove",      START, "g1", "f3",
              "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1");
    applyTest("Capture resets halfmove",              "4k3/8/8/8/8/5n2/8/4KR2 w - - 5 10", "f1", "f3",
              "4k3/8/8/8/8/5R2/8/4K3 b - - 0 10");
    applyTest("White O-O",                            "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1", "g1",
              "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1");
    applyTest("White O-O-O",                          "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1", "c1",
              "r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1");
    applyTest("Black O-O",                            "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", "e8", "g8",
              "r4rk1/8/8/8/8/8/8/R3K2R w KQ - 1 2");
    applyTest("Black O-O-O",                          "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", "e8", "c8",
              "2kr3r/8/8/8/8/8/8/R3K2R w KQ - 1 2");
    applyTest("Rook h1 move loses K right",           "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "h1", "h4",
              "r3k2r/8/8/8/7R/8/8/R3K3 b Qkq - 1 1");
    applyTest("Capturing rook on a8 removes q right", "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "a1", "a8",
              "R3k2r/8/8/8/8/8/8/4K2R b Kk - 0 1");
    applyTest("White en passant exd6",                "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5", "d6",
              "4k3/8/3P4/8/8/8/8/4K3 b - - 0 1");
    applyTest("Black en passant dxe3",                "4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1", "d4", "e3",
              "4k3/8/8/8/8/4p3/8/4K3 w - - 0 2");
    applyTest("White promotion to queen",             "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7", "a8",
              "Q3k3/8/8/8/8/8/8/4K3 b - - 0 1");
    applyTest("Black promotion with capture",         "4k3/8/8/8/8/8/6p1/4K2R b K - 0 1", "g2", "h1",
              "4k3/8/8/8/8/8/8/4K2q w - - 0 2");

    printf("\n--- inCheck ---\n");
    checkTest("Starting position: no check",          START, Color::white, false);
    checkTest("Rook on file",                         "4k3/8/8/8/8/8/8/4R1K1 b - - 0 1", Color::black, true);
    checkTest("Rook blocked",                         "4k3/4p3/8/8/8/8/8/4R1K1 b - - 0 1", Color::black, false);
    checkTest("Rook on rank",                         "8/8/8/8/8/8/8/r3K3 w - - 0 1", Color::white, true);
    checkTest("No rank wrap (king h2, rook a1)",      "4k3/8/8/8/8/8/7K/r7 w - - 0 1", Color::white, false);
    checkTest("Bishop up-right",                      "4k3/8/8/8/7b/8/8/4K3 w - - 0 1", Color::white, true);
    checkTest("Bishop up-left",                       "4k3/8/b7/8/8/8/4K3/8 w - - 0 1", Color::white, true);
    checkTest("Bishop down-right",                    "4k3/8/8/7B/8/8/8/4K3 b - - 0 1", Color::black, true);
    checkTest("Bishop down-left",                     "4k3/8/8/8/B7/8/8/4K3 b - - 0 1", Color::black, true);
    checkTest("Bishop blocked",                       "4k3/8/8/b7/8/2P5/8/4K3 w - - 0 1", Color::white, false);
    checkTest("No diagonal wrap (king h4, bishop a4)","4k3/8/8/8/b6K/8/8/8 w - - 0 1", Color::white, false);
    checkTest("Queen diagonal",                       "4k3/8/8/8/8/8/3q4/4K3 w - - 0 1", Color::white, true);
    checkTest("Knight check",                         "4k3/8/3N4/8/8/8/8/6K1 b - - 0 1", Color::black, true);
    checkTest("No knight wrap (king a1, knight g2)",  "4k3/8/8/8/8/8/6n1/K7 w - - 0 1", Color::white, false);
    checkTest("White pawn checks black king",         "4k3/3P4/8/8/8/8/8/6K1 b - - 0 1", Color::black, true);
    checkTest("Pawn straight ahead doesn't check",    "4k3/4P3/8/8/8/8/8/6K1 b - - 0 1", Color::black, false);
    checkTest("Black pawn checks white king",         "4k3/8/8/8/8/3p4/4K3/8 w - - 0 1", Color::white, true);
    checkTest("Black pawn behind king doesn't check", "4k3/8/8/8/8/8/4K3/3p4 w - - 0 1", Color::white, false);
    checkTest("Adjacent kings",                       "8/8/8/8/8/8/3k4/4K3 w - - 0 1", Color::white, true);
    checkTest("Own rook doesn't check",               "4k3/8/8/8/8/8/8/R3K3 w - - 0 1", Color::white, false);
    checkTest("King in corner h1, nothing near",      "4k3/8/8/8/8/8/8/7K w - - 0 1", Color::white, false);
    checkTest("King in corner a8, nothing near",      "k7/8/8/8/8/8/8/7K b - - 0 1", Color::black, false);

    printf("\n--- validPieceMovement ---\n");
    shapeTest("Knight g1-f3",                         START, "g1", "f3", true);
    shapeTest("Knight g1-g3",                         START, "g1", "g3", false);
    shapeTest("Knight b1-h3 (wrap) rejected",         "4k3/8/8/8/8/8/8/1N2K3 w - - 0 1", "b1", "h3", false);
    shapeTest("Bishop c1-h6",                         "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1", "c1", "h6", true);
    shapeTest("Bishop c1-d3 rejected",                "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1", "c1", "d3", false);
    shapeTest("Rook a1-a8",                           "4k3/8/8/8/8/8/8/R3K3 w - - 0 1", "a1", "a8", true);
    shapeTest("Rook a1-b2 rejected",                  "4k3/8/8/8/8/8/8/R3K3 w - - 0 1", "a1", "b2", false);
    shapeTest("Queen d1-h5",                          "4k3/8/8/8/8/8/8/3QK3 w - - 0 1", "d1", "h5", true);
    shapeTest("Queen d1-d8",                          "4k3/8/8/8/8/8/8/3QK3 w - - 0 1", "d1", "d8", true);
    shapeTest("Queen d1-e3 rejected",                 "4k3/8/8/8/8/8/8/3QK3 w - - 0 1", "d1", "e3", false);
    shapeTest("King e1-f2",                           "4k3/8/8/8/8/8/8/4K3 w - - 0 1", "e1", "f2", true);
    shapeTest("King e1-e3 rejected",                  "4k3/8/8/8/8/8/8/4K3 w - - 0 1", "e1", "e3", false);
    shapeTest("King h2-a1 (wrap) rejected",           "4k3/8/8/8/8/8/7K/8 w - - 0 1", "h2", "a1", false);
    shapeTest("White pawn e2-e3",                     START, "e2", "e3", true);
    shapeTest("White pawn e2-e4",                     START, "e2", "e4", true);
    shapeTest("White pawn e3-e5 rejected",            "4k3/8/8/8/8/4P3/8/4K3 w - - 0 1", "e3", "e5", false);
    shapeTest("White pawn backward rejected",         "4k3/8/8/8/8/4P3/8/4K3 w - - 0 1", "e3", "e2", false);
    shapeTest("White pawn push into piece rejected",  "4k3/8/8/8/8/4n3/4P3/4K3 w - - 0 1", "e2", "e3", false);
    shapeTest("White pawn diagonal empty rejected",   START, "e2", "d3", false);
    shapeTest("White pawn capture",                   "4k3/8/8/8/8/3n4/4P3/4K3 w - - 0 1", "e2", "d3", true);
    shapeTest("White pawn h2-a2 (wrap) rejected",     "4k3/8/8/8/8/8/n6P/4K3 w - - 0 1", "h2", "a2", false);
    shapeTest("White en passant",                     "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5", "d6", true);
    shapeTest("Black pawn e7-e5",                     "4k3/4p3/8/8/8/8/8/4K3 b - - 0 1", "e7", "e5", true);
    shapeTest("Black pawn backward rejected",         "4k3/8/4p3/8/8/8/8/4K3 b - - 0 1", "e6", "e7", false);
    shapeTest("Black pawn a7-h7 (wrap) rejected",     "4k3/p6N/8/8/8/8/8/4K3 b - - 0 1", "a7", "h7", false);
    shapeTest("Black en passant",                     "4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1", "d4", "e3", true);

    printf("\n--- openPath ---\n");
    pathTest("Rook a1-a8 clear",                      "4k3/8/8/8/8/8/8/R3K3 w - - 0 1", "a1", "a8", true);
    pathTest("Rook a1-a8 blocked on a4",              "4k3/8/8/8/p7/8/8/R3K3 w - - 0 1", "a1", "a8", false);
    pathTest("Rook a1-a4 onto the blocker",           "4k3/8/8/8/p7/8/8/R3K3 w - - 0 1", "a1", "a4", true);
    pathTest("Rook sideways blocked",                 "4k3/8/8/8/8/8/8/RN2K3 w - - 0 1", "a1", "d1", false);
    pathTest("Bishop c1-h6 blocked on e3",            "4k3/8/8/8/8/4p3/8/2B1K3 w - - 0 1", "c1", "h6", false);
    pathTest("Queen d1-h5 clear",                     "4k3/8/8/8/8/8/8/3QK3 w - - 0 1", "d1", "h5", true);
    pathTest("Pawn double push blocked on e3",        "4k3/8/8/8/8/4n3/4P3/4K3 w - - 0 1", "e2", "e4", false);
    pathTest("Knight ignores blockers",               START, "g1", "f3", true);

    printf("\n--- tryMove ---\n");
    moveTest("e2-e4 legal",                           START, "e2", "e4", MoveType::legal);
    moveTest("Knight g1-f3 legal",                    START, "g1", "f3", MoveType::legal);
    moveTest("Black piece on white's turn",           START, "e7", "e5", MoveType::wrongTurn);
    moveTest("Empty from-square",                     START, "e4", "e5", MoveType::wrongTurn);
    moveTest("Capturing own piece",                   START, "b1", "d2", MoveType::selfCapture);
    moveTest("Bishop moving straight",                "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1", "c1", "c4", MoveType::wrongMovement);
    moveTest("Rook path blocked",                     "4k3/8/8/8/p7/8/8/R3K3 w - - 0 1", "a1", "a8", MoveType::blockedPath);
    moveTest("Pinned rook leaves file",               "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1", "e2", "d2", MoveType::leavesKingInCheck);
    moveTest("Pinned rook moves along pin",           "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1", "e2", "e8", MoveType::legal);
    moveTest("King steps into rook's file",           "3r2k1/8/8/8/8/8/8/4K3 w - - 0 1", "e1", "d1", MoveType::leavesKingInCheck);
    moveTest("King steps next to enemy king",         "8/8/8/8/8/2k5/8/4K3 w - - 0 1", "e1", "d2", MoveType::leavesKingInCheck);
    moveTest("Ignoring check",                        "4r1k1/8/8/8/8/8/P7/4K3 w - - 0 1", "a2", "a3", MoveType::leavesKingInCheck);
    moveTest("Blocking check",                        "4r1k1/8/8/8/8/8/3R4/4K3 w - - 0 1", "d2", "e2", MoveType::legal);
    moveTest("Capturing the checking piece",          "4k3/8/8/8/8/8/4r3/3QK3 w - - 0 1", "d1", "e2", MoveType::legal);
    moveTest("En passant legal",                      "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5", "d6", MoveType::legal);
    moveTest("En passant exposing king (illegal)",    "8/8/8/K2pP2r/8/8/8/4k3 w - d6 0 1", "e5", "d6", MoveType::leavesKingInCheck);
    moveTest("Promotion legal",                       "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", "a7", "a8", MoveType::legal);
    moveTest("White O-O legal",                       "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1", "g1", MoveType::legal);
    moveTest("White O-O-O legal",                     "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1", "c1", MoveType::legal);
    moveTest("O-O without the right",                 "r3k2r/8/8/8/8/8/8/R3K2R w Qkq - 0 1", "e1", "g1", MoveType::illegalCastle);
    moveTest("O-O blocked by bishop f1",              "r3k2r/8/8/8/8/8/8/R3KB1R w KQkq - 0 1", "e1", "g1", MoveType::illegalCastle);
    moveTest("O-O-O blocked by knight b1",            "r3k2r/8/8/8/8/8/8/RN2K2R w KQkq - 0 1", "e1", "c1", MoveType::illegalCastle);
    moveTest("Castling out of check",                 "4r1k1/8/8/8/8/8/8/R3K2R w KQ - 0 1", "e1", "g1", MoveType::illegalCastle);
    moveTest("Castling through attacked f1",          "5rk1/8/8/8/8/8/8/R3K2R w KQ - 0 1", "e1", "g1", MoveType::illegalCastle);
    moveTest("Bishop knight-shaped move",             "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1", "c1", "d3", MoveType::wrongMovement);
    unchangedOnRejectTest("Rejected move leaves position alone", START, "e2", "e5");

    printf("\n--- occupancy ---\n");
    {
        Position p;
        p.fromFEN(START);
        uint64_t expected = 0x000000000000FFFFULL | 0xFFFF000000000000ULL;  // rows 0-1 and 6-7
        report("Starting position occupancy", p.occupancy() == expected, "ranks 1, 2, 7, 8 set", "different bits");
        p.fromFEN("4k3/8/8/8/8/8/8/7K w - - 0 1");
        report("Only e8 (bit 4) and h1 (bit 63)", p.occupancy() == ((1ULL << 4) | (1ULL << 63)),
               "bits 4 and 63", "different bits");
    }

    printf("\n%d of %d tests passed\n", testsPassed, testsRun);
    return testsPassed == testsRun ? 0 : 1;
}

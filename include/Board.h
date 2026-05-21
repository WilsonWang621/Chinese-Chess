#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include "Types.h"

#include <array>
#include <vector>

class Board {
public:
    static const int Rows = 10;
    static const int Cols = 9;

    Board();

    struct Undo {
        Move move;
        Piece moved;
        Piece captured;
    };

    void reset();
    void print() const;
    bool inBounds(Position pos) const;
    Piece at(Position pos) const;
    bool applyMove(const Move &move);
    Undo makeMove(const Move &move);
    void unmakeMove(const Undo &undo);
    std::vector<Move> ruleMoves(Side side) const;
    std::vector<Move> legalMoves(Side side) const;
    std::vector<Move> legalMovesFast(Side side);
    bool isRuleMove(const Move &move, Side side) const;
    bool isLegalMove(const Move &move, Side side) const;
    bool isInCheck(Side side) const;
    bool isSquareAttacked(Position pos, Side bySide) const;
    bool hasGeneral(Side side) const;

private:
    std::array<std::array<Piece, Cols>, Rows> cells_;

    void setupBackRank(int row, Side side);
    char symbol(Piece piece) const;
    Position findGeneral(Side side) const;
    bool canLand(Position to, Side side) const;
    bool inPalace(Position pos, Side side) const;
    bool crossedRiver(Position pos, Side side) const;
    bool clearLine(Position from, Position to) const;
    void addIfLegalLanding(std::vector<Move> &moves, Position from, Position to) const;
    std::vector<Move> pseudoMoves(Position from) const;
    void addGeneralMoves(std::vector<Move> &moves, Position from, Side side) const;
    void addAdvisorMoves(std::vector<Move> &moves, Position from, Side side) const;
    void addElephantMoves(std::vector<Move> &moves, Position from, Side side) const;
    void addHorseMoves(std::vector<Move> &moves, Position from, Side side) const;
    void addChariotMoves(std::vector<Move> &moves, Position from, Side side) const;
    void addCannonMoves(std::vector<Move> &moves, Position from, Side side) const;
    void addSlidingMoves(std::vector<Move> &moves, Position from, Side side, bool cannon) const;
    void addSoldierMoves(std::vector<Move> &moves, Position from, Side side) const;
};

#endif

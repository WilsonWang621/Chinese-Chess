#ifndef CHESS_SIMPLE_AI_H
#define CHESS_SIMPLE_AI_H

#include "Board.h"

#include <random>

class SimpleAI {
public:
    SimpleAI();

    Move chooseMove(const Board &board, Side side);

private:
    std::mt19937 engine_;

    int moveScore(const Board &board, const Move &move, Side side) const;
    int strategicSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const;
    int tacticalSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const;
    int evaluatePosition(const Board &board, Side side) const;
    int evaluateMaterial(const Board &board, Side side) const;
    int positionalValue(Piece piece, Position pos, Side side) const;
    int threatScore(const Board &board, Side side) const;
    int moveOrderScore(const Board &board, const Move &move, Side movingSide, Side aiSide) const;
    std::vector<Move> orderedMoves(const Board &board, Side side, Side aiSide, int limit, bool tacticalOnly) const;
    bool isCapture(const Board &board, const Move &move, Side side) const;
    bool givesCheck(const Board &board, const Move &move, Side side) const;
    int pieceValue(PieceType type) const;
};

#endif

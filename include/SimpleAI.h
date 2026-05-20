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

    int pieceValue(PieceType type) const;
};

#endif

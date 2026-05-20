#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include "Board.h"
#include "SimpleAI.h"

class Game {
public:
    void run();

private:
    Board board_;
    SimpleAI ai_;

    bool humanTurn();
    void clearLine();
};

#endif

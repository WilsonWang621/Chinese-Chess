#include "SimpleAI.h"

#include <ctime>
#include <limits>

SimpleAI::SimpleAI() : engine_(static_cast<unsigned int>(std::time(nullptr))) {}

Move SimpleAI::chooseMove(const Board &board, Side side) {
    std::vector<Move> moves = board.legalMoves(side);
    if (moves.empty()) {
        return {};
    }

    int bestScore = std::numeric_limits<int>::min();
    std::vector<Move> bestMoves;
    for (const Move &move : moves) {
        int score = pieceValue(board.at(move.to).type);
        if (score > bestScore) {
            bestScore = score;
            bestMoves.clear();
            bestMoves.push_back(move);
        } else if (score == bestScore) {
            bestMoves.push_back(move);
        }
    }

    std::uniform_int_distribution<std::size_t> dist(0, bestMoves.size() - 1);
    return bestMoves[dist(engine_)];
}

int SimpleAI::pieceValue(PieceType type) const {
    switch (type) {
        case PieceType::General: return 10000;
        case PieceType::Chariot: return 500;
        case PieceType::Cannon: return 350;
        case PieceType::Horse: return 300;
        case PieceType::Elephant: return 120;
        case PieceType::Advisor: return 120;
        case PieceType::Soldier: return 80;
        case PieceType::Empty: return 0;
    }
    return 0;
}

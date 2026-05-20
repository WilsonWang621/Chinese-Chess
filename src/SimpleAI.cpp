#include "SimpleAI.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <limits>

namespace {
const int WinScore = 1000000;
const int MateScore = 900000;
const int StrategicDepth = 2;
const int StrategicMoveLimit = 6;
const int TacticalDepth = 4;
const int TacticalMoveLimit = 6;
}

SimpleAI::SimpleAI() : engine_(static_cast<unsigned int>(std::time(nullptr))) {}

Move SimpleAI::chooseMove(const Board &board, Side side) {
    std::vector<Move> moves = board.legalMoves(side);
    if (moves.empty()) {
        return {};
    }

    int bestScore = std::numeric_limits<int>::min();
    std::vector<Move> bestMoves;
    for (const Move &move : moves) {
        int score = moveScore(board, move, side);
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

int SimpleAI::moveScore(const Board &board, const Move &move, Side side) const {
    Board next = board;
    next.applyMove(move);
    return strategicSearch(next, opponent(side), side, StrategicDepth - 1,
                           std::numeric_limits<int>::min(),
                           std::numeric_limits<int>::max());
}

int SimpleAI::strategicSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const {
    if (!board.hasGeneral(aiSide)) {
        return -WinScore - depth;
    }
    if (!board.hasGeneral(opponent(aiSide))) {
        return WinScore + depth;
    }

    std::vector<Move> legal = board.legalMoves(turn);
    if (legal.empty()) {
        if (board.isInCheck(turn)) {
            return turn == aiSide ? -MateScore - depth : MateScore + depth;
        }
        return evaluatePosition(board, aiSide);
    }

    if (depth <= 0) {
        return tacticalSearch(board, turn, aiSide, TacticalDepth, alpha, beta);
    }

    std::vector<Move> moves = orderedMoves(board, turn, aiSide, StrategicMoveLimit, false);
    int bestScore = turn == aiSide ? std::numeric_limits<int>::min()
                                   : std::numeric_limits<int>::max();
    for (const Move &move : moves) {
        Board next = board;
        next.applyMove(move);
        int score = strategicSearch(next, opponent(turn), aiSide, depth - 1, alpha, beta);

        if (turn == aiSide) {
            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, bestScore);
        } else {
            bestScore = std::min(bestScore, score);
            beta = std::min(beta, bestScore);
        }

        if (beta <= alpha) {
            break;
        }
    }
    return bestScore;
}

int SimpleAI::tacticalSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const {
    if (depth <= 0 || !board.hasGeneral(Side::Red) || !board.hasGeneral(Side::Black)) {
        return evaluatePosition(board, aiSide);
    }

    std::vector<Move> moves = orderedMoves(board, turn, aiSide, TacticalMoveLimit, true);
    if (moves.empty()) {
        return evaluatePosition(board, aiSide);
    }

    int bestScore = turn == aiSide ? std::numeric_limits<int>::min()
                                   : std::numeric_limits<int>::max();
    for (const Move &move : moves) {
        Board next = board;
        next.applyMove(move);
        int score = tacticalSearch(next, opponent(turn), aiSide, depth - 1, alpha, beta);

        if (turn == aiSide) {
            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, bestScore);
        } else {
            bestScore = std::min(bestScore, score);
            beta = std::min(beta, bestScore);
        }

        if (beta <= alpha) {
            break;
        }
    }
    return bestScore;
}

int SimpleAI::evaluatePosition(const Board &board, Side side) const {
    if (!board.hasGeneral(side)) {
        return -WinScore;
    }
    if (!board.hasGeneral(opponent(side))) {
        return WinScore;
    }

    int score = evaluateMaterial(board, side);
    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Piece piece = board.at({row, col});
            if (piece.side == side) {
                score += positionalValue(piece, {row, col}, side);
            } else if (piece.side == opponent(side)) {
                score -= positionalValue(piece, {row, col}, opponent(side));
            }
        }
    }

    score += threatScore(board, side) * 2;
    score -= threatScore(board, opponent(side)) * 3;
    if (board.isInCheck(opponent(side))) {
        score += 260;
    }
    if (board.isInCheck(side)) {
        score -= 360;
    }
    return score;
}

int SimpleAI::evaluateMaterial(const Board &board, Side side) const {
    int score = 0;
    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Piece piece = board.at({row, col});
            if (piece.side == side) {
                score += pieceValue(piece.type);
            } else if (piece.side == opponent(side)) {
                score -= pieceValue(piece.type);
            }
        }
    }
    return score;
}

int SimpleAI::positionalValue(Piece piece, Position pos, Side side) const {
    int forward = side == Side::Red ? 9 - pos.row : pos.row;
    int center = 4 - std::abs(pos.col - 4);
    switch (piece.type) {
        case PieceType::General:
            return 10 - std::abs(pos.col - 4) * 4;
        case PieceType::Advisor:
        case PieceType::Elephant:
            return 8;
        case PieceType::Horse:
            return center * 8 + forward * 3;
        case PieceType::Chariot:
            return center * 5 + forward * 2;
        case PieceType::Cannon:
            return center * 7 + forward * 2;
        case PieceType::Soldier:
            return forward * 10 + (forward >= 5 ? center * 6 + 25 : 0);
        case PieceType::Empty:
            return 0;
    }
    return 0;
}

int SimpleAI::threatScore(const Board &board, Side side) const {
    std::vector<int> threats;
    for (const Move &move : board.legalMoves(side)) {
        int score = 0;
        Piece attacker = board.at(move.from);
        Piece target = board.at(move.to);
        if (target.type != PieceType::Empty && target.side == opponent(side)) {
            score += pieceValue(target.type) - pieceValue(attacker.type) / 8;
        }
        if (givesCheck(board, move, side)) {
            score += 180;
        }
        if (score > 0) {
            threats.push_back(score);
        }
    }

    std::sort(threats.begin(), threats.end(), std::greater<int>());
    int score = 0;
    for (std::size_t index = 0; index < threats.size() && index < 3; ++index) {
        score += threats[index] / static_cast<int>(index + 1);
    }
    return score;
}

int SimpleAI::moveOrderScore(const Board &board, const Move &move, Side movingSide, Side aiSide) const {
    Piece attacker = board.at(move.from);
    Piece target = board.at(move.to);
    int score = 0;
    if (target.type != PieceType::Empty && target.side == opponent(movingSide)) {
        score += pieceValue(target.type) * 12 - pieceValue(attacker.type);
    }
    if (givesCheck(board, move, movingSide)) {
        score += 2200;
    }

    Board next = board;
    next.applyMove(move);
    score += evaluateMaterial(next, aiSide) / 10;
    if (next.isInCheck(movingSide)) {
        score -= 500;
    }
    return score;
}

std::vector<Move> SimpleAI::orderedMoves(const Board &board, Side side, Side aiSide, int limit, bool tacticalOnly) const {
    struct ScoredMove {
        Move move;
        int score;
    };

    std::vector<ScoredMove> scoredMoves;
    bool mustAnswerCheck = board.isInCheck(side);
    for (const Move &move : board.legalMoves(side)) {
        if (!tacticalOnly || mustAnswerCheck || isCapture(board, move, side)) {
            scoredMoves.push_back({move, moveOrderScore(board, move, side, aiSide)});
        }
    }

    std::sort(scoredMoves.begin(), scoredMoves.end(), [](const ScoredMove &left, const ScoredMove &right) {
        return left.score > right.score;
    });
    if (limit > 0 && scoredMoves.size() > static_cast<std::size_t>(limit)) {
        scoredMoves.resize(static_cast<std::size_t>(limit));
    }

    std::vector<Move> moves;
    for (const ScoredMove &scoredMove : scoredMoves) {
        moves.push_back(scoredMove.move);
    }
    return moves;
}

bool SimpleAI::isCapture(const Board &board, const Move &move, Side side) const {
    Piece target = board.at(move.to);
    return target.type != PieceType::Empty && target.side == opponent(side);
}

bool SimpleAI::givesCheck(const Board &board, const Move &move, Side side) const {
    Board next = board;
    next.applyMove(move);
    return next.isInCheck(opponent(side));
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

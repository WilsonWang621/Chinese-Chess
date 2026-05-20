#ifndef CHESS_SIMPLE_AI_H
#define CHESS_SIMPLE_AI_H

#include "Board.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <random>
#include <unordered_map>

class SimpleAI {
public:
    SimpleAI();

    Move chooseMove(const Board &board, Side side);

private:
    enum class Bound {
        Exact,
        Lower,
        Upper
    };

    struct TranspositionEntry {
        int depth = 0;
        int score = 0;
        int bestMove = -1;
        Bound bound = Bound::Exact;
    };

    std::mt19937 engine_;
    std::uint64_t zobrist_[3][8][Board::Rows][Board::Cols];
    std::uint64_t zobristTurn_[3];
    mutable std::unordered_map<std::uint64_t, TranspositionEntry> transposition_;
    mutable std::array<std::array<std::array<int, Board::Rows * Board::Cols>, Board::Rows * Board::Cols>, 3> history_;
    mutable std::chrono::steady_clock::time_point deadline_;
    mutable bool searchStopped_ = false;

    int moveScore(const Board &board, const Move &move, Side side, int depth) const;
    int strategicSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const;
    int tacticalSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const;
    int evaluatePosition(const Board &board, Side side) const;
    int evaluateMaterial(const Board &board, Side side) const;
    int positionalValue(Piece piece, Position pos, Side side) const;
    int threatScore(const Board &board, Side side) const;
    int weightedTacticalPatternScore(const Board &board, Side side) const;
    int tacticalOpportunityScale(const Board &board, Side side) const;
    int tacticalPatternScore(const Board &board, Side side) const;
    int palaceAttackScore(const Board &board, Side side) const;
    int linePressureScore(const Board &board, Side side) const;
    int mobilityLockScore(const Board &board, Side side) const;
    int moveOrderScore(const Board &board, const Move &move, Side movingSide, Side aiSide, int ttBestMove) const;
    int hangingMovePenalty(const Board &board, const Move &move, Side movingSide) const;
    std::vector<Move> orderedMoves(const Board &board, Side side, Side aiSide, int limit, bool tacticalOnly) const;
    bool isCapture(const Board &board, const Move &move, Side side) const;
    bool givesCheck(const Board &board, const Move &move, Side side) const;
    bool timeExpired() const;
    Position findGeneralPosition(const Board &board, Side side) const;
    bool samePosition(Position left, Position right) const;
    bool inPalaceArea(Position pos, Side palaceSide) const;
    int linePiecesBetween(const Board &board, Position from, Position to) const;
    std::uint64_t boardHash(const Board &board, Side turn, Side aiSide) const;
    int encodeMove(const Move &move) const;
    int sideIndex(Side side) const;
    int pieceIndex(PieceType type) const;
    int squareIndex(Position pos) const;
    int pieceValue(PieceType type) const;
};

#endif

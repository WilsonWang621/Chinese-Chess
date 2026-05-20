#include "SimpleAI.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <limits>
#include <utility>

namespace {
const int WinScore = 1000000;
const int MateScore = 900000;
const int RootMoveLimit = 12;
const int MaxIterativeDepth = 5;
const int MoveTimeMs = 1600;
const int StrategicMoveLimit = 10;
const int TacticalDepth = 5;
const int TacticalMoveLimit = 10;
}

SimpleAI::SimpleAI() : engine_(static_cast<unsigned int>(std::time(nullptr))) {
    std::uniform_int_distribution<std::uint64_t> dist;
    for (int side = 0; side < 3; ++side) {
        zobristTurn_[side] = dist(engine_);
        for (int type = 0; type < 8; ++type) {
            for (int row = 0; row < Board::Rows; ++row) {
                for (int col = 0; col < Board::Cols; ++col) {
                    zobrist_[side][type][row][col] = dist(engine_);
                }
            }
        }
    }
    for (auto &sideHistory : history_) {
        for (auto &fromHistory : sideHistory) {
            fromHistory.fill(0);
        }
    }
}

Move SimpleAI::chooseMove(const Board &board, Side side) {
    std::vector<Move> moves = board.legalMoves(side);
    if (moves.empty()) {
        return {};
    }

    deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(MoveTimeMs);
    searchStopped_ = false;
    if (transposition_.size() > 250000) {
        transposition_.clear();
    }

    std::vector<Move> bestMoves{moves.front()};
    std::vector<Move> candidates = orderedMoves(board, side, side, RootMoveLimit, false);

    for (int depth = 1; depth <= MaxIterativeDepth; ++depth) {
        int bestScore = std::numeric_limits<int>::min();
        std::vector<Move> depthBestMoves;

        for (const Move &move : candidates) {
            if (timeExpired()) {
                searchStopped_ = true;
                break;
            }

            int score = moveScore(board, move, side, depth);
            if (searchStopped_) {
                break;
            }

            if (score > bestScore) {
                bestScore = score;
                depthBestMoves.clear();
                depthBestMoves.push_back(move);
            } else if (score == bestScore) {
                depthBestMoves.push_back(move);
            }
        }

        if (searchStopped_ || depthBestMoves.empty()) {
            break;
        }

        bestMoves = depthBestMoves;
        std::stable_sort(candidates.begin(), candidates.end(), [&](const Move &left, const Move &right) {
            bool leftBest = encodeMove(left) == encodeMove(bestMoves.front());
            bool rightBest = encodeMove(right) == encodeMove(bestMoves.front());
            return leftBest && !rightBest;
        });
    }

    std::uniform_int_distribution<std::size_t> dist(0, bestMoves.size() - 1);
    return bestMoves[dist(engine_)];
}

int SimpleAI::moveScore(const Board &board, const Move &move, Side side, int depth) const {
    Board next = board;
    next.applyMove(move);
    return strategicSearch(next, opponent(side), side, depth - 1,
                           std::numeric_limits<int>::min(),
                           std::numeric_limits<int>::max());
}

int SimpleAI::strategicSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const {
    if (timeExpired()) {
        searchStopped_ = true;
        return evaluatePosition(board, aiSide);
    }
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

    int originalAlpha = alpha;
    int originalBeta = beta;
    std::uint64_t hash = boardHash(board, turn, aiSide);
    int ttBestMove = -1;
    auto found = transposition_.find(hash);
    if (found != transposition_.end()) {
        const TranspositionEntry &entry = found->second;
        ttBestMove = entry.bestMove;
        if (entry.depth >= depth) {
            if (entry.bound == Bound::Exact) {
                return entry.score;
            }
            if (entry.bound == Bound::Lower) {
                alpha = std::max(alpha, entry.score);
            } else if (entry.bound == Bound::Upper) {
                beta = std::min(beta, entry.score);
            }
            if (alpha >= beta) {
                return entry.score;
            }
        }
    }

    std::vector<Move> moves = orderedMoves(board, turn, aiSide, StrategicMoveLimit, false);
    int bestScore = turn == aiSide ? std::numeric_limits<int>::min()
                                   : std::numeric_limits<int>::max();
    int bestMove = -1;
    for (const Move &move : moves) {
        Board next = board;
        next.applyMove(move);
        int score = strategicSearch(next, opponent(turn), aiSide, depth - 1, alpha, beta);
        if (searchStopped_) {
            return evaluatePosition(board, aiSide);
        }

        if (turn == aiSide) {
            if (score > bestScore) {
                bestScore = score;
                bestMove = encodeMove(move);
            }
            alpha = std::max(alpha, bestScore);
        } else {
            if (score < bestScore) {
                bestScore = score;
                bestMove = encodeMove(move);
            }
            beta = std::min(beta, bestScore);
        }

        if (beta <= alpha) {
            if (!isCapture(board, move, turn)) {
                history_[sideIndex(turn)][squareIndex(move.from)][squareIndex(move.to)] += depth * depth;
            }
            break;
        }
    }

    if (!searchStopped_) {
        Bound bound = Bound::Exact;
        if (bestScore <= originalAlpha) {
            bound = Bound::Upper;
        } else if (bestScore >= originalBeta) {
            bound = Bound::Lower;
        }
        transposition_[hash] = {depth, bestScore, bestMove == -1 ? ttBestMove : bestMove, bound};
    }
    return bestScore;
}

int SimpleAI::tacticalSearch(const Board &board, Side turn, Side aiSide, int depth, int alpha, int beta) const {
    if (timeExpired()) {
        searchStopped_ = true;
        return evaluatePosition(board, aiSide);
    }
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
        if (searchStopped_) {
            return evaluatePosition(board, aiSide);
        }

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
    score += tacticalPatternScore(board, side);
    score -= tacticalPatternScore(board, opponent(side)) * 2;
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

int SimpleAI::tacticalPatternScore(const Board &board, Side side) const {
    return palaceAttackScore(board, side) +
           linePressureScore(board, side) +
           mobilityLockScore(board, side);
}

int SimpleAI::palaceAttackScore(const Board &board, Side side) const {
    Side enemy = opponent(side);
    Position enemyGeneral = findGeneralPosition(board, enemy);
    if (!board.inBounds(enemyGeneral)) {
        return WinScore / 2;
    }

    int score = 0;
    int attackCount = 0;
    int checkingPieces = 0;
    int horsePressure = 0;
    int cannonPressure = 0;
    int chariotPressure = 0;
    int soldierPressure = 0;
    int advisors = 0;

    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Piece piece = board.at({row, col});
            if (piece.side == enemy && piece.type == PieceType::Advisor && inPalaceArea({row, col}, enemy)) {
                ++advisors;
            }
        }
    }

    for (const Move &move : board.legalMoves(side)) {
        Piece attacker = board.at(move.from);
        bool inEnemyPalace = inPalaceArea(move.to, enemy);
        bool attacksGeneral = samePosition(move.to, enemyGeneral);
        if (inEnemyPalace || attacksGeneral) {
            ++attackCount;
            score += 24;
        }
        if (attacksGeneral) {
            ++checkingPieces;
            score += 180;
        }
        if (inEnemyPalace) {
            switch (attacker.type) {
                case PieceType::Horse:
                    horsePressure += 1;
                    score += 95;
                    break;
                case PieceType::Cannon:
                    cannonPressure += 1;
                    score += 115;
                    break;
                case PieceType::Chariot:
                    chariotPressure += 1;
                    score += 120;
                    break;
                case PieceType::Soldier:
                    soldierPressure += 1;
                    score += 85;
                    break;
                default:
                    break;
            }
        }
    }

    if (advisors < 2) {
        score += (2 - advisors) * 140;
    }
    if (horsePressure > 0 && cannonPressure > 0) {
        score += 260;
    }
    if (cannonPressure >= 2) {
        score += 280;
    }
    if (chariotPressure > 0 && cannonPressure > 0) {
        score += 240;
    }
    if (chariotPressure >= 2) {
        score += 220;
    }
    if (soldierPressure > 0 && (cannonPressure > 0 || chariotPressure > 0)) {
        score += 180;
    }
    if (checkingPieces >= 2 || attackCount >= 4) {
        score += 220;
    }
    return score;
}

int SimpleAI::linePressureScore(const Board &board, Side side) const {
    Side enemy = opponent(side);
    Position enemyGeneral = findGeneralPosition(board, enemy);
    if (!board.inBounds(enemyGeneral)) {
        return WinScore / 2;
    }

    int score = 0;
    int centerCannons = 0;
    int bottomCannons = 0;
    int sameFileCannonPressure = 0;

    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Position pos{row, col};
            Piece piece = board.at(pos);
            if (piece.side != side) {
                continue;
            }
            bool sameFile = pos.col == enemyGeneral.col;
            bool sameRank = pos.row == enemyGeneral.row;
            if (!sameFile && !sameRank) {
                continue;
            }

            int between = linePiecesBetween(board, pos, enemyGeneral);
            if (piece.type == PieceType::Cannon) {
                if (sameFile && between == 1) {
                    score += 230;
                    ++sameFileCannonPressure;
                } else if (between == 1) {
                    score += 140;
                }
                if (sameFile && std::abs(pos.row - enemyGeneral.row) >= 3) {
                    ++centerCannons;
                }
                if (inPalaceArea({enemy == Side::Red ? 9 : 0, pos.col}, enemy) && sameFile) {
                    ++bottomCannons;
                }
            } else if (piece.type == PieceType::Chariot && between == 0) {
                score += 220;
            }
        }
    }

    if (sameFileCannonPressure >= 2 || (centerCannons > 0 && bottomCannons > 0)) {
        score += 320;
    }
    return score;
}

int SimpleAI::mobilityLockScore(const Board &board, Side side) const {
    Side enemy = opponent(side);
    int score = 0;

    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Position pos{row, col};
            Piece piece = board.at(pos);
            if (piece.side != enemy || (piece.type != PieceType::Chariot && piece.type != PieceType::Horse)) {
                continue;
            }

            int mobility = 0;
            for (const Move &move : board.legalMoves(enemy)) {
                if (samePosition(move.from, pos)) {
                    ++mobility;
                }
            }
            bool undeveloped = enemy == Side::Red ? row >= 7 : row <= 2;
            if (mobility == 0) {
                score += piece.type == PieceType::Chariot ? 180 : 120;
            } else if (mobility == 1 && undeveloped) {
                score += piece.type == PieceType::Chariot ? 100 : 70;
            }
        }
    }
    return score;
}

int SimpleAI::moveOrderScore(const Board &board, const Move &move, Side movingSide, Side aiSide, int ttBestMove) const {
    Piece attacker = board.at(move.from);
    Piece target = board.at(move.to);
    int score = 0;
    if (encodeMove(move) == ttBestMove) {
        score += 100000;
    }
    if (target.type != PieceType::Empty && target.side == opponent(movingSide)) {
        score += pieceValue(target.type) * 16 - pieceValue(attacker.type);
    }
    if (givesCheck(board, move, movingSide)) {
        score += 5000;
    }

    Board next = board;
    next.applyMove(move);
    score += evaluateMaterial(next, aiSide) / 10;
    int patternGain = tacticalPatternScore(next, movingSide) - tacticalPatternScore(board, movingSide);
    int enemyPatternDrop = tacticalPatternScore(board, opponent(movingSide)) - tacticalPatternScore(next, opponent(movingSide));
    score += patternGain * 2 + enemyPatternDrop * 3;
    if (next.isInCheck(movingSide)) {
        score -= 500;
    }
    if (target.type == PieceType::Empty) {
        score += history_[sideIndex(movingSide)][squareIndex(move.from)][squareIndex(move.to)];
    }
    return score;
}

std::vector<Move> SimpleAI::orderedMoves(const Board &board, Side side, Side aiSide, int limit, bool tacticalOnly) const {
    struct ScoredMove {
        Move move;
        int score;
    };

    std::vector<ScoredMove> scoredMoves;
    int ttBestMove = -1;
    auto found = transposition_.find(boardHash(board, side, aiSide));
    if (found != transposition_.end()) {
        ttBestMove = found->second.bestMove;
    }

    bool mustAnswerCheck = board.isInCheck(side);
    for (const Move &move : board.legalMoves(side)) {
        if (!tacticalOnly || mustAnswerCheck || isCapture(board, move, side)) {
            scoredMoves.push_back({move, moveOrderScore(board, move, side, aiSide, ttBestMove)});
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

bool SimpleAI::timeExpired() const {
    return std::chrono::steady_clock::now() >= deadline_;
}

Position SimpleAI::findGeneralPosition(const Board &board, Side side) const {
    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Piece piece = board.at({row, col});
            if (piece.side == side && piece.type == PieceType::General) {
                return {row, col};
            }
        }
    }
    return {-1, -1};
}

bool SimpleAI::samePosition(Position left, Position right) const {
    return left.row == right.row && left.col == right.col;
}

bool SimpleAI::inPalaceArea(Position pos, Side palaceSide) const {
    if (pos.col < 3 || pos.col > 5) {
        return false;
    }
    return palaceSide == Side::Red ? pos.row >= 7 && pos.row <= 9
                                   : pos.row >= 0 && pos.row <= 2;
}

int SimpleAI::linePiecesBetween(const Board &board, Position from, Position to) const {
    if (from.row != to.row && from.col != to.col) {
        return -1;
    }
    int dr = (to.row > from.row) - (to.row < from.row);
    int dc = (to.col > from.col) - (to.col < from.col);
    Position cur{from.row + dr, from.col + dc};
    int count = 0;
    while (cur.row != to.row || cur.col != to.col) {
        if (board.at(cur).type != PieceType::Empty) {
            ++count;
        }
        cur.row += dr;
        cur.col += dc;
    }
    return count;
}

std::uint64_t SimpleAI::boardHash(const Board &board, Side turn, Side aiSide) const {
    std::uint64_t hash = zobristTurn_[sideIndex(turn)] ^ (zobristTurn_[sideIndex(aiSide)] << 1);
    for (int row = 0; row < Board::Rows; ++row) {
        for (int col = 0; col < Board::Cols; ++col) {
            Piece piece = board.at({row, col});
            if (piece.type != PieceType::Empty) {
                hash ^= zobrist_[sideIndex(piece.side)][pieceIndex(piece.type)][row][col];
            }
        }
    }
    return hash;
}

int SimpleAI::encodeMove(const Move &move) const {
    return squareIndex(move.from) * Board::Rows * Board::Cols + squareIndex(move.to);
}

int SimpleAI::sideIndex(Side side) const {
    switch (side) {
        case Side::Red: return 1;
        case Side::Black: return 2;
        case Side::None: return 0;
    }
    return 0;
}

int SimpleAI::pieceIndex(PieceType type) const {
    switch (type) {
        case PieceType::General: return 1;
        case PieceType::Advisor: return 2;
        case PieceType::Elephant: return 3;
        case PieceType::Horse: return 4;
        case PieceType::Chariot: return 5;
        case PieceType::Cannon: return 6;
        case PieceType::Soldier: return 7;
        case PieceType::Empty: return 0;
    }
    return 0;
}

int SimpleAI::squareIndex(Position pos) const {
    return pos.row * Board::Cols + pos.col;
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

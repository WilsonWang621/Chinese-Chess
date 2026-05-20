#include "Board.h"

#include <algorithm>
#include <cctype>
#include <iostream>

Board::Board() {
    reset();
}

void Board::reset() {
    for (auto &row : cells_) {
        row.fill(Piece{});
    }

    setupBackRank(0, Side::Black);
    cells_[2][1] = {PieceType::Cannon, Side::Black};
    cells_[2][7] = {PieceType::Cannon, Side::Black};
    for (int col = 0; col < Cols; col += 2) {
        cells_[3][col] = {PieceType::Soldier, Side::Black};
    }

    setupBackRank(9, Side::Red);
    cells_[7][1] = {PieceType::Cannon, Side::Red};
    cells_[7][7] = {PieceType::Cannon, Side::Red};
    for (int col = 0; col < Cols; col += 2) {
        cells_[6][col] = {PieceType::Soldier, Side::Red};
    }
}

void Board::print() const {
    std::cout << "\n    0  1  2  3  4  5  6  7  8\n";
    std::cout << "  +---------------------------+\n";
    for (int row = 0; row < Rows; ++row) {
        std::cout << row << " |";
        for (int col = 0; col < Cols; ++col) {
            std::cout << ' ' << symbol(cells_[row][col]) << ' ';
        }
        std::cout << "|\n";
        if (row == 4) {
            std::cout << "  |--------- river -----------|\n";
        }
    }
    std::cout << "  +---------------------------+\n";
}

bool Board::inBounds(Position pos) const {
    return pos.row >= 0 && pos.row < Rows && pos.col >= 0 && pos.col < Cols;
}

Piece Board::at(Position pos) const {
    return cells_[pos.row][pos.col];
}

bool Board::applyMove(const Move &move) {
    if (!inBounds(move.from) || !inBounds(move.to)) {
        return false;
    }
    cells_[move.to.row][move.to.col] = cells_[move.from.row][move.from.col];
    cells_[move.from.row][move.from.col] = {};
    return true;
}

std::vector<Move> Board::ruleMoves(Side side) const {
    std::vector<Move> result;
    for (int row = 0; row < Rows; ++row) {
        for (int col = 0; col < Cols; ++col) {
            Position from{row, col};
            if (at(from).side != side) {
                continue;
            }
            for (const Move &move : pseudoMoves(from)) {
                result.push_back(move);
            }
        }
    }
    return result;
}

std::vector<Move> Board::legalMoves(Side side) const {
    std::vector<Move> result;
    for (int row = 0; row < Rows; ++row) {
        for (int col = 0; col < Cols; ++col) {
            Position from{row, col};
            if (at(from).side != side) {
                continue;
            }
            for (const Move &move : pseudoMoves(from)) {
                Board next = *this;
                next.applyMove(move);
                if (!next.isInCheck(side)) {
                    result.push_back(move);
                }
            }
        }
    }
    return result;
}

bool Board::isRuleMove(const Move &move, Side side) const {
    std::vector<Move> moves = ruleMoves(side);
    return std::any_of(moves.begin(), moves.end(), [&](const Move &candidate) {
        return candidate.from.row == move.from.row &&
               candidate.from.col == move.from.col &&
               candidate.to.row == move.to.row &&
               candidate.to.col == move.to.col;
    });
}

bool Board::isLegalMove(const Move &move, Side side) const {
    std::vector<Move> moves = legalMoves(side);
    return std::any_of(moves.begin(), moves.end(), [&](const Move &candidate) {
        return candidate.from.row == move.from.row &&
               candidate.from.col == move.from.col &&
               candidate.to.row == move.to.row &&
               candidate.to.col == move.to.col;
    });
}

bool Board::isInCheck(Side side) const {
    Position general = findGeneral(side);
    if (!inBounds(general)) {
        return true;
    }
    Side enemy = opponent(side);
    for (int row = 0; row < Rows; ++row) {
        for (int col = 0; col < Cols; ++col) {
            Position from{row, col};
            if (at(from).side != enemy) {
                continue;
            }
            for (const Move &move : pseudoMoves(from)) {
                if (move.to.row == general.row && move.to.col == general.col) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool Board::hasGeneral(Side side) const {
    return inBounds(findGeneral(side));
}

void Board::setupBackRank(int row, Side side) {
    cells_[row][0] = {PieceType::Chariot, side};
    cells_[row][1] = {PieceType::Horse, side};
    cells_[row][2] = {PieceType::Elephant, side};
    cells_[row][3] = {PieceType::Advisor, side};
    cells_[row][4] = {PieceType::General, side};
    cells_[row][5] = {PieceType::Advisor, side};
    cells_[row][6] = {PieceType::Elephant, side};
    cells_[row][7] = {PieceType::Horse, side};
    cells_[row][8] = {PieceType::Chariot, side};
}

char Board::symbol(Piece piece) const {
    if (piece.type == PieceType::Empty) {
        return '.';
    }

    char ch = '?';
    switch (piece.type) {
        case PieceType::General: ch = 'K'; break;
        case PieceType::Advisor: ch = 'A'; break;
        case PieceType::Elephant: ch = 'E'; break;
        case PieceType::Horse: ch = 'H'; break;
        case PieceType::Chariot: ch = 'R'; break;
        case PieceType::Cannon: ch = 'C'; break;
        case PieceType::Soldier: ch = 'S'; break;
        case PieceType::Empty: ch = '.'; break;
    }
    return piece.side == Side::Red ? ch : static_cast<char>(std::tolower(ch));
}

Position Board::findGeneral(Side side) const {
    for (int row = 0; row < Rows; ++row) {
        for (int col = 0; col < Cols; ++col) {
            Piece piece = cells_[row][col];
            if (piece.side == side && piece.type == PieceType::General) {
                return {row, col};
            }
        }
    }
    return {-1, -1};
}

bool Board::canLand(Position to, Side side) const {
    return inBounds(to) && at(to).side != side;
}

bool Board::inPalace(Position pos, Side side) const {
    if (pos.col < 3 || pos.col > 5) {
        return false;
    }
    return side == Side::Red ? pos.row >= 7 && pos.row <= 9
                             : pos.row >= 0 && pos.row <= 2;
}

bool Board::crossedRiver(Position pos, Side side) const {
    return side == Side::Red ? pos.row <= 4 : pos.row >= 5;
}

bool Board::clearLine(Position from, Position to) const {
    int dr = (to.row > from.row) - (to.row < from.row);
    int dc = (to.col > from.col) - (to.col < from.col);
    Position cur{from.row + dr, from.col + dc};
    while (cur.row != to.row || cur.col != to.col) {
        if (at(cur).type != PieceType::Empty) {
            return false;
        }
        cur.row += dr;
        cur.col += dc;
    }
    return true;
}

void Board::addIfLegalLanding(std::vector<Move> &moves, Position from, Position to) const {
    if (canLand(to, at(from).side)) {
        moves.push_back({from, to});
    }
}

std::vector<Move> Board::pseudoMoves(Position from) const {
    Piece piece = at(from);
    std::vector<Move> moves;
    if (piece.type == PieceType::Empty) {
        return moves;
    }

    switch (piece.type) {
        case PieceType::General:
            addGeneralMoves(moves, from, piece.side);
            break;
        case PieceType::Advisor:
            addAdvisorMoves(moves, from, piece.side);
            break;
        case PieceType::Elephant:
            addElephantMoves(moves, from, piece.side);
            break;
        case PieceType::Horse:
            addHorseMoves(moves, from, piece.side);
            break;
        case PieceType::Chariot:
            addChariotMoves(moves, from, piece.side);
            break;
        case PieceType::Cannon:
            addCannonMoves(moves, from, piece.side);
            break;
        case PieceType::Soldier:
            addSoldierMoves(moves, from, piece.side);
            break;
        case PieceType::Empty:
            break;
    }
    return moves;
}

void Board::addGeneralMoves(std::vector<Move> &moves, Position from, Side side) const {
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto &dir : dirs) {
        Position to{from.row + dir[0], from.col + dir[1]};
        if (inPalace(to, side) && canLand(to, side)) {
            moves.push_back({from, to});
        }
    }

    Position enemy = findGeneral(opponent(side));
    if (enemy.col == from.col && clearLine(from, enemy)) {
        moves.push_back({from, enemy});
    }
}

void Board::addAdvisorMoves(std::vector<Move> &moves, Position from, Side side) const {
    const int dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (const auto &dir : dirs) {
        Position to{from.row + dir[0], from.col + dir[1]};
        if (inPalace(to, side) && canLand(to, side)) {
            moves.push_back({from, to});
        }
    }
}

void Board::addElephantMoves(std::vector<Move> &moves, Position from, Side side) const {
    const int dirs[4][2] = {{2, 2}, {2, -2}, {-2, 2}, {-2, -2}};
    for (const auto &dir : dirs) {
        Position eye{from.row + dir[0] / 2, from.col + dir[1] / 2};
        Position to{from.row + dir[0], from.col + dir[1]};
        bool ownSide = side == Side::Red ? to.row >= 5 : to.row <= 4;
        if (inBounds(to) && ownSide && at(eye).type == PieceType::Empty && canLand(to, side)) {
            moves.push_back({from, to});
        }
    }
}

void Board::addHorseMoves(std::vector<Move> &moves, Position from, Side side) const {
    const int jumps[8][4] = {
        {-2, -1, -1, 0}, {-2, 1, -1, 0}, {2, -1, 1, 0}, {2, 1, 1, 0},
        {-1, -2, 0, -1}, {1, -2, 0, -1}, {-1, 2, 0, 1}, {1, 2, 0, 1}
    };
    for (const auto &jump : jumps) {
        Position leg{from.row + jump[2], from.col + jump[3]};
        Position to{from.row + jump[0], from.col + jump[1]};
        if (inBounds(to) && at(leg).type == PieceType::Empty && canLand(to, side)) {
            moves.push_back({from, to});
        }
    }
}

void Board::addChariotMoves(std::vector<Move> &moves, Position from, Side side) const {
    addSlidingMoves(moves, from, side, false);
}

void Board::addCannonMoves(std::vector<Move> &moves, Position from, Side side) const {
    addSlidingMoves(moves, from, side, true);
}

void Board::addSlidingMoves(std::vector<Move> &moves, Position from, Side side, bool cannon) const {
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto &dir : dirs) {
        bool jumped = false;
        Position to{from.row + dir[0], from.col + dir[1]};
        while (inBounds(to)) {
            Piece target = at(to);
            if (!cannon) {
                if (target.type == PieceType::Empty) {
                    moves.push_back({from, to});
                } else {
                    if (target.side != side) {
                        moves.push_back({from, to});
                    }
                    break;
                }
            } else {
                if (!jumped) {
                    if (target.type == PieceType::Empty) {
                        moves.push_back({from, to});
                    } else {
                        jumped = true;
                    }
                } else if (target.type != PieceType::Empty) {
                    if (target.side != side) {
                        moves.push_back({from, to});
                    }
                    break;
                }
            }
            to.row += dir[0];
            to.col += dir[1];
        }
    }
}

void Board::addSoldierMoves(std::vector<Move> &moves, Position from, Side side) const {
    int forward = side == Side::Red ? -1 : 1;
    addIfLegalLanding(moves, from, {from.row + forward, from.col});
    if (crossedRiver(from, side)) {
        addIfLegalLanding(moves, from, {from.row, from.col - 1});
        addIfLegalLanding(moves, from, {from.row, from.col + 1});
    }
}

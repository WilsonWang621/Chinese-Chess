#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

enum class Side {
    None,
    Red,
    Black
};

enum class PieceType {
    Empty,
    General,
    Advisor,
    Elephant,
    Horse,
    Chariot,
    Cannon,
    Soldier
};

struct Piece {
    PieceType type = PieceType::Empty;
    Side side = Side::None;
};

struct Position {
    int row = 0;
    int col = 0;
};

struct Move {
    Position from;
    Position to;
};

Side opponent(Side side);

#endif

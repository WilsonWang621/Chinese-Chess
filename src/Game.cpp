#include "Game.h"

#include <iostream>
#include <limits>
#include <string>

void Game::run() {
    std::cout << "Chinese Chess: human Red (uppercase) vs AI Black (lowercase)\n";
    std::cout << "Input: from_row from_col to_row to_col, for example: 6 0 5 0\n";
    std::cout << "Type q to quit.\n";

    while (true) {
        board_.print();
        if (!board_.hasGeneral(Side::Red)) {
            std::cout << "AI wins.\n";
            return;
        }
        if (!board_.hasGeneral(Side::Black)) {
            std::cout << "You win.\n";
            return;
        }

        if (board_.legalMoves(Side::Red).empty()) {
            std::cout << "You have no legal moves. AI wins.\n";
            return;
        }

        if (!humanTurn()) {
            return;
        }

        if (!board_.hasGeneral(Side::Black) || board_.legalMoves(Side::Black).empty()) {
            board_.print();
            std::cout << "You win.\n";
            return;
        }

        Move aiMove = ai_.chooseMove(board_, Side::Black);
        board_.applyMove(aiMove);
        std::cout << "AI move: " << aiMove.from.row << ' ' << aiMove.from.col
                  << " -> " << aiMove.to.row << ' ' << aiMove.to.col << "\n";
    }
}

bool Game::humanTurn() {
    while (true) {
        std::cout << "Your move> ";
        std::string first;
        if (!(std::cin >> first)) {
            return false;
        }
        if (first == "q" || first == "Q") {
            return false;
        }

        Move move;
        try {
            move.from.row = std::stoi(first);
        } catch (...) {
            clearLine();
            std::cout << "Invalid input.\n";
            continue;
        }

        if (!(std::cin >> move.from.col >> move.to.row >> move.to.col)) {
            std::cin.clear();
            clearLine();
            std::cout << "Invalid input.\n";
            continue;
        }

        if (!board_.isLegalMove(move, Side::Red)) {
            std::cout << "Illegal move.\n";
            continue;
        }

        board_.applyMove(move);
        return true;
    }
}

void Game::clearLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

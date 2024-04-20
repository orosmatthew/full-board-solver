#pragma once

#include "full_board_game.hpp"

inline void solve_step(FullBoardGame& game)
{
    auto start = Direction::north;
    while (true) {
        for (int i = dir_idx(start); i < 4; ++i) {
            if (game.move(idx_dir(i)).record.has_value()) {
                return;
            }
        }
        std::optional<Direction> next;
        do {
            const Direction last = game.last_move()->dir;
            if (!game.undo()) {
                return;
            }
            next = next_dir(last);
        } while (!next.has_value());
        start = *next;
    }
}
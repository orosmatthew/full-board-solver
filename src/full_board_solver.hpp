#pragma once

#include "full_board_game.hpp"

inline std::optional<Vector2i> next_pos(const FullBoardGame& game, const Vector2i prev)
{
    int i = game.pos_to_idx(prev);
    while (i < game.size() * game.size() - 1) {
        i++;
        if (!game.barrier_at(game.idx_to_pos(i))) {
            return game.idx_to_pos(i);
        }
    }
    return std::nullopt;
};

inline void solve_step_local(FullBoardGame& game)
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

inline std::optional<Vector2i> first_avail_pos(const FullBoardGame& game)
{
    if (const std::optional<Vector2i> next = next_pos(game, game.idx_to_pos(-1)); next.has_value()) {
        return *next;
    }
    return std::nullopt;
}

enum class AutoSolveResult { should_continue, should_stop };

inline AutoSolveResult auto_solve_update(FullBoardGame& game, std::optional<std::chrono::milliseconds> solve_time)
{
    if (!game.start_pos().has_value()) {
        if (const std::optional<Vector2i> pos = first_avail_pos(game); pos.has_value()) {
            game.set_start(pos.value());
        }
        else {
            return AutoSolveResult::should_stop;
        }
    }
    const auto start_time = std::chrono::steady_clock::now();
    const std::optional<std::chrono::time_point<std::chrono::steady_clock>> target_time
        = solve_time.has_value() ? std::optional(start_time + solve_time.value()) : std::nullopt;
    do {
        solve_step_local(game);
        if (game.move_history().empty()) {
            const std::optional<Vector2i> next = next_pos(game, game.start_pos().value());
            game.reset_leave_barriers();
            if (next.has_value()) {
                game.set_start(next.value());
            }
            else {
                return AutoSolveResult::should_stop;
            }
        }
    } while (!game.won() && game.start_pos().has_value() && target_time.has_value()
             && std::chrono::steady_clock::now() < target_time.value());
    if (game.won()) {
        return AutoSolveResult::should_stop;
    }
    return AutoSolveResult::should_continue;
}
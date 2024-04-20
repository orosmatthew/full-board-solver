#include <optional>

#include <raylib-cpp.hpp>

#include "common.hpp"
#include "full_board_game.hpp"
#include "full_board_solver.hpp"

namespace rl = raylib;

constexpr int c_window_width = 800;
constexpr int c_window_height = 800;

enum class GameState { manual, solving };

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    const rl::Window window(c_window_width, c_window_height, "Full Board Solver");

    constexpr int square_padding = 4;
    FullBoardGame game(6);
    const int square_size_px = c_window_width / game.size() - 2 * square_padding;
    const int grid_square_size = c_window_width / game.size();

    auto state = GameState::manual;

    while (!window.ShouldClose()) {

        if (state == GameState::manual) {
            if (IsKeyPressed(KEY_R)) {
                game.reset_leave_barriers();
            }

            if (IsKeyPressed(KEY_U)) {
                game.undo();
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !game.result().has_value()) {
                const rl::Vector2 mouse_pos = GetMousePosition();
                const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / grid_square_size),
                                            static_cast<int>(mouse_pos.y / grid_square_size) };
                if (game.current_pos().has_value()) {
                    if (grid_pos.x == game.current_pos()->x && grid_pos.y != game.current_pos()->y) {
                        game.move(grid_pos.y > game.current_pos()->y ? Direction::south : Direction::north);
                    }
                    else if (grid_pos.x != game.current_pos()->x && grid_pos.y == game.current_pos()->y) {
                        game.move(grid_pos.x > game.current_pos()->x ? Direction::east : Direction::west);
                    }
                }
                else {
                    game.set_start(grid_pos);
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !game.start_pos().has_value()) {
                const rl::Vector2 mouse_pos = GetMousePosition();
                const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / grid_square_size),
                                            static_cast<int>(mouse_pos.y / grid_square_size) };
                game.toggle_barrier(grid_pos);
            }

            if (IsKeyPressed(KEY_S) && game.current_pos().has_value()) {
                solve_step(game);
            }
            if (IsKeyPressed(KEY_A)) {
                state = GameState::solving;
            }
        }
        else {
            auto next_pos = [&](const Vector2i prev) {
                int i = game.pos_to_idx(prev);
                while (i < game.size() * game.size()) {
                    i++;
                    if (std::ranges::find(game.barrier_positions(), game.idx_to_pos(i))
                        == game.barrier_positions().end()) {
                        break;
                    }
                }
                return game.idx_to_pos(i);
            };
            if (!game.start_pos().has_value()) {
                const Vector2i next = next_pos(game.idx_to_pos(-1));
                game.set_start(next);
            }
            solve_step(game);
            if (game.move_history().empty()) {
                int i = game.pos_to_idx(game.start_pos().value());
                game.reset_leave_barriers();
                if (i >= game.size() * game.size()) {
                    state = GameState::manual;
                }
                else {
                    const Vector2i next = next_pos(*game.start_pos());
                    game.set_start(next);
                }
            }
            if (IsKeyPressed(KEY_A)
                || (game.result().has_value() && game.result().value() == FullBoardGame::Result::won)) {
                state = GameState::manual;
            }
        }

        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        for (int x = 0; x < game.size(); ++x) {
            for (int y = 0; y < game.size(); ++y) {
                DrawRectangle(
                    square_padding + x * grid_square_size,
                    square_padding + y * grid_square_size,
                    square_size_px,
                    square_size_px,
                    rl::Color(168, 168, 168));
                if (game.filled_at({ x, y })) {
                    DrawCircle(
                        grid_square_size / 2 + x * grid_square_size,
                        grid_square_size / 2 + y * grid_square_size,
                        static_cast<float>(square_size_px) / 2,
                        game.result().has_value()
                            ? game.result().value() == FullBoardGame::Result::won ? DARKGREEN : RED
                            : GRAY);
                }
            }
        }
        for (auto [dir, from, to] : game.move_history()) {
            DrawLineEx(
                { grid_square_size / 2.0f + static_cast<float>(from.x) * grid_square_size,
                  grid_square_size / 2.0f + static_cast<float>(from.y) * grid_square_size },
                { grid_square_size / 2.0f + static_cast<float>(to.x) * grid_square_size,
                  grid_square_size / 2.0f + static_cast<float>(to.y) * grid_square_size },
                5.0f,
                BLUE);
        }
        for (const auto [x, y] : game.barrier_positions()) {
            DrawCircle(
                grid_square_size / 2 + x * grid_square_size,
                grid_square_size / 2 + y * grid_square_size,
                static_cast<float>(square_size_px) / 2,
                BLACK);
        }
        if (game.start_pos().has_value()) {
            DrawCircle(
                grid_square_size / 2 + game.start_pos()->x * grid_square_size,
                grid_square_size / 2 + game.start_pos()->y * grid_square_size,
                static_cast<float>(square_size_px) / 4,
                BLUE);
        }
        if (game.current_pos().has_value()) {
            DrawCircle(
                grid_square_size / 2 + game.current_pos()->x * grid_square_size,
                grid_square_size / 2 + game.current_pos()->y * grid_square_size,
                static_cast<float>(square_size_px) / 2,
                BLUE);
        }
        EndDrawing();
    }
}
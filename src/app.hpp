#pragma once

#include <raylib-cpp.hpp>

#include "common.hpp"
#include "full_board_solver.hpp"

enum class GameState { manual, solving };

class App {
public:
    App()
        : m_window(c_window_width, c_window_height, "Full Board Solver")
        , m_game(9)
        , m_square_size(static_cast<float>(c_window_width) / static_cast<float>(m_game.size()) - 2 * c_square_padding)
        , m_grid_square_size(static_cast<float>(c_window_width) / static_cast<float>(m_game.size()))
        , m_state(GameState::manual)
    {
    }

    [[nodiscard]] bool should_close() const
    {
        return m_window.ShouldClose();
    }

    void update()
    {

        if (m_state == GameState::manual) {
            if (IsKeyPressed(KEY_R)) {
                m_game.reset_leave_barriers();
            }

            if (IsKeyPressed(KEY_U)) {
                m_game.undo();
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !m_game.result().has_value()) {
                const raylib::Vector2 mouse_pos = GetMousePosition();
                const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / m_grid_square_size),
                                            static_cast<int>(mouse_pos.y / m_grid_square_size) };
                if (m_game.current_pos().has_value()) {
                    if (grid_pos.x == m_game.current_pos()->x && grid_pos.y != m_game.current_pos()->y) {
                        m_game.move(grid_pos.y > m_game.current_pos()->y ? Direction::south : Direction::north);
                    }
                    else if (grid_pos.x != m_game.current_pos()->x && grid_pos.y == m_game.current_pos()->y) {
                        m_game.move(grid_pos.x > m_game.current_pos()->x ? Direction::east : Direction::west);
                    }
                }
                else {
                    m_game.set_start(grid_pos);
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !m_game.start_pos().has_value()) {
                const raylib::Vector2 mouse_pos = GetMousePosition();
                const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / m_grid_square_size),
                                            static_cast<int>(mouse_pos.y / m_grid_square_size) };
                m_game.toggle_barrier(grid_pos);
            }

            if (IsKeyPressed(KEY_S) && m_game.current_pos().has_value()) {
                solve_step(m_game);
            }
            if (IsKeyPressed(KEY_A)) {
                m_state = GameState::solving;
            }
        }
        else {
            auto next_pos = [&](const Vector2i prev) {
                int i = m_game.pos_to_idx(prev);
                while (i < m_game.size() * m_game.size()) {
                    i++;
                    if (std::ranges::find(m_game.barrier_positions(), m_game.idx_to_pos(i))
                        == m_game.barrier_positions().end()) {
                        break;
                    }
                }
                return m_game.idx_to_pos(i);
            };
            if (!m_game.start_pos().has_value()) {
                const Vector2i next = next_pos(m_game.idx_to_pos(-1));
                m_game.set_start(next);
            }
            solve_step(m_game);
            if (m_game.move_history().empty()) {
                const int i = m_game.pos_to_idx(m_game.start_pos().value());
                m_game.reset_leave_barriers();
                if (i >= m_game.size() * m_game.size()) {
                    m_state = GameState::manual;
                }
                else {
                    const Vector2i next = next_pos(*m_game.start_pos());
                    m_game.set_start(next);
                }
            }
            if (IsKeyPressed(KEY_A)
                || (m_game.result().has_value() && m_game.result().value() == FullBoardGame::Result::won)) {
                m_state = GameState::manual;
            }
        }

        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        for (int x = 0; x < m_game.size(); ++x) {
            for (int y = 0; y < m_game.size(); ++y) {
                DrawRectangle(
                    static_cast<int>(c_square_padding + static_cast<float>(x) * m_grid_square_size),
                    static_cast<int>(c_square_padding + static_cast<float>(y) * m_grid_square_size),
                    static_cast<int>(m_square_size),
                    static_cast<int>(m_square_size),
                    raylib::Color(168, 168, 168));
                if (m_game.filled_at({ x, y })) {
                    DrawCircle(
                        static_cast<int>(m_grid_square_size / 2 + static_cast<float>(x) * m_grid_square_size),
                        static_cast<int>(m_grid_square_size / 2 + static_cast<float>(y) * m_grid_square_size),
                        m_square_size / 2,
                        m_game.result().has_value()
                            ? m_game.result().value() == FullBoardGame::Result::won ? DARKGREEN : RED
                            : GRAY);
                }
            }
        }
        for (auto [dir, from, to] : m_game.move_history()) {
            DrawLineEx(
                { m_grid_square_size / 2.0f + static_cast<float>(from.x) * m_grid_square_size,
                  m_grid_square_size / 2.0f + static_cast<float>(from.y) * m_grid_square_size },
                { m_grid_square_size / 2.0f + static_cast<float>(to.x) * m_grid_square_size,
                  m_grid_square_size / 2.0f + static_cast<float>(to.y) * m_grid_square_size },
                5.0f,
                BLUE);
        }
        for (const auto [x, y] : m_game.barrier_positions()) {
            DrawCircle(
                static_cast<int>(m_grid_square_size / 2 + static_cast<float>(x) * m_grid_square_size),
                static_cast<int>(m_grid_square_size / 2 + static_cast<float>(y) * m_grid_square_size),
                m_square_size / 2,
                BLACK);
        }
        if (m_game.start_pos().has_value()) {
            DrawCircle(
                static_cast<int>(
                    m_grid_square_size / 2 + static_cast<float>(m_game.start_pos()->x) * m_grid_square_size),
                static_cast<int>(
                    m_grid_square_size / 2 + static_cast<float>(m_game.start_pos()->y) * m_grid_square_size),
                m_square_size / 4,
                BLUE);
        }
        if (m_game.current_pos().has_value()) {
            DrawCircle(
                static_cast<int>(
                    m_grid_square_size / 2 + static_cast<float>(m_game.current_pos()->x) * m_grid_square_size),
                static_cast<int>(
                    m_grid_square_size / 2 + static_cast<float>(m_game.current_pos()->y) * m_grid_square_size),
                m_square_size / 2,
                BLUE);
        }
        EndDrawing();
    }

private:
    static constexpr int c_window_width = 800;
    static constexpr int c_window_height = 800;
    static constexpr float c_square_padding = 4;
    raylib::Window m_window;
    FullBoardGame m_game;
    float m_square_size;
    float m_grid_square_size;
    GameState m_state;
};
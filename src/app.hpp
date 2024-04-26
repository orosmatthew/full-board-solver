#pragma once

#include <chrono>

#include <raylib-cpp.hpp>

#include "common.hpp"
#include "full_board_solver.hpp"

enum class GameState { manual, solving };

class App {
public:
    App()
        : m_window(600, 600, "Full Board Solver")
        , m_game(5)
        , m_board_sizes(calc_board_sizes())
        , m_state(GameState::manual)
    {
    }

    [[nodiscard]] bool should_close() const
    {
        return m_window.ShouldClose();
    }

    void update()
    {
        if (IsWindowResized()) {
            m_board_sizes = calc_board_sizes();
        }
        if (m_state == GameState::manual) {
            update_manual();
        }
        else {
            update_solving();
        }
    }

    void draw() const
    {
        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        for (int x = 0; x < m_game.size(); ++x) {
            for (int y = 0; y < m_game.size(); ++y) {
                DrawRectangle(
                    static_cast<int>(
                        m_board_sizes.x_offset + m_board_sizes.square_padding
                        + static_cast<float>(x) * m_board_sizes.grid_square),
                    static_cast<int>(m_board_sizes.square_padding + static_cast<float>(y) * m_board_sizes.grid_square),
                    static_cast<int>(m_board_sizes.inner_square),
                    static_cast<int>(m_board_sizes.inner_square),
                    raylib::Color(168, 168, 168));
                if (m_game.filled_at({ x, y })) {
                    DrawCircle(
                        static_cast<int>(
                            m_board_sizes.x_offset + m_board_sizes.grid_square / 2
                            + static_cast<float>(x) * m_board_sizes.grid_square),
                        static_cast<int>(
                            m_board_sizes.grid_square / 2 + static_cast<float>(y) * m_board_sizes.grid_square),
                        m_board_sizes.inner_square / 2,
                        m_game.result().has_value()
                            ? m_game.result().value() == FullBoardGame::Result::won ? DARKGREEN : RED
                            : GRAY);
                }
            }
        }
        for (auto [dir, from, to] : m_game.move_history()) {
            DrawLineEx(
                { m_board_sizes.x_offset + m_board_sizes.grid_square / 2.0f
                      + static_cast<float>(from.x) * m_board_sizes.grid_square,
                  m_board_sizes.grid_square / 2.0f + static_cast<float>(from.y) * m_board_sizes.grid_square },
                { m_board_sizes.x_offset + m_board_sizes.grid_square / 2.0f
                      + static_cast<float>(to.x) * m_board_sizes.grid_square,
                  m_board_sizes.grid_square / 2.0f + static_cast<float>(to.y) * m_board_sizes.grid_square },
                5.0f,
                BLUE);
        }
        for (const auto [x, y] : m_game.barrier_positions()) {
            DrawCircle(
                static_cast<int>(
                    m_board_sizes.x_offset + m_board_sizes.grid_square / 2
                    + static_cast<float>(x) * m_board_sizes.grid_square),
                static_cast<int>(m_board_sizes.grid_square / 2 + static_cast<float>(y) * m_board_sizes.grid_square),
                m_board_sizes.inner_square / 2,
                BLACK);
        }
        if (m_game.start_pos().has_value()) {
            DrawCircle(
                static_cast<int>(
                    m_board_sizes.x_offset + m_board_sizes.grid_square / 2
                    + static_cast<float>(m_game.start_pos()->x) * m_board_sizes.grid_square),
                static_cast<int>(
                    m_board_sizes.grid_square / 2
                    + static_cast<float>(m_game.start_pos()->y) * m_board_sizes.grid_square),
                m_board_sizes.inner_square / 4,
                BLUE);
        }
        if (m_game.current_pos().has_value()) {
            DrawCircle(
                static_cast<int>(
                    m_board_sizes.x_offset + m_board_sizes.grid_square / 2
                    + static_cast<float>(m_game.current_pos()->x) * m_board_sizes.grid_square),
                static_cast<int>(
                    m_board_sizes.grid_square / 2
                    + static_cast<float>(m_game.current_pos()->y) * m_board_sizes.grid_square),
                m_board_sizes.inner_square / 2,
                BLUE);
        }
        EndDrawing();
    }

private:
    struct BoardSizes {
        float x_offset;
        float grid_square;
        float square_padding;
        float inner_square;
    };

    [[nodiscard]] BoardSizes calc_board_sizes() const
    {
        const Vector2i screen_size { GetScreenWidth(), GetScreenHeight() };
        const auto min_size = static_cast<float>(std::min(screen_size.x, screen_size.y));
        BoardSizes sizes {};
        sizes.x_offset = (static_cast<float>(screen_size.x) - min_size) / 2.0f;
        sizes.grid_square = min_size / static_cast<float>(m_game.size());
        sizes.square_padding = 0.05f * sizes.grid_square;
        sizes.inner_square = min_size / static_cast<float>(m_game.size()) - 2 * sizes.square_padding;
        return sizes;
    }

    [[nodiscard]] std::optional<Vector2i> mouse_to_grid() const
    {
        const raylib::Vector2 mouse_pos = GetMousePosition();
        if (const Vector2i grid_pos
            = { static_cast<int>((mouse_pos.x - m_board_sizes.x_offset) / m_board_sizes.grid_square),
                static_cast<int>(mouse_pos.y / m_board_sizes.grid_square) };
            m_game.in_bounds(grid_pos)) {
            return grid_pos;
        }
        return {};
    }

    void update_manual()
    {
        if (IsKeyPressed(KEY_R)) {
            m_game.reset_leave_barriers();
        }

        if (IsKeyPressed(KEY_U)) {
            m_game.undo();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !m_game.result().has_value()) {
            auto in_diff_column = [this](const Vector2i pos) {
                return pos.x == m_game.current_pos()->x && pos.y != m_game.current_pos()->y;
            };
            auto in_diff_row = [this](const Vector2i pos) {
                return pos.x != m_game.current_pos()->x && pos.y == m_game.current_pos()->y;
            };
            if (const std::optional<Vector2i> grid_pos = mouse_to_grid(); grid_pos.has_value()) {
                if (m_game.current_pos().has_value()) {
                    if (in_diff_column(*grid_pos)) {
                        m_game.move(grid_pos->y > m_game.current_pos()->y ? Direction::south : Direction::north);
                    }
                    else if (in_diff_row(*grid_pos)) {
                        m_game.move(grid_pos->x > m_game.current_pos()->x ? Direction::east : Direction::west);
                    }
                }
                else {
                    m_game.set_start(*grid_pos);
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !m_game.start_pos().has_value()) {
            if (const std::optional<Vector2i> grid_pos = mouse_to_grid(); grid_pos.has_value()) {
                m_game.toggle_barrier(*grid_pos);
            }
        }

        if (IsKeyPressed(KEY_S) && m_game.current_pos().has_value()) {
            solve_step(m_game);
        }
        if (IsKeyPressed(KEY_A)) {
            m_state = GameState::solving;
        }

        if (IsKeyPressed(KEY_UP)) {
            set_game_size(m_game.size() + 1);
        }
        else if (IsKeyPressed(KEY_DOWN)) {
            set_game_size(m_game.size() - 1);
        }
    }

    void update_solving()
    {
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
        const auto start_time = std::chrono::steady_clock::now();
        const auto target_time = start_time + std::chrono::milliseconds(16);
        do {
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
        } while (!m_game.won() && m_game.start_pos().has_value() && std::chrono::steady_clock::now() < target_time);

        if (IsKeyPressed(KEY_A)
            || (m_game.result().has_value() && m_game.result().value() == FullBoardGame::Result::won)) {
            m_state = GameState::manual;
        }
    }

    void set_game_size(int new_size)
    {
        if (m_state == GameState::manual) {
            new_size = std::clamp(new_size, 1, 100);
            m_game = FullBoardGame(new_size);
            m_board_sizes = calc_board_sizes();
        }
    }

    static constexpr Vector2i c_init_window_size = { 800, 800 };
    raylib::Window m_window;
    FullBoardGame m_game;
    BoardSizes m_board_sizes;
    GameState m_state;
};
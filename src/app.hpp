#pragma once

#include <chrono>

#include <raygui.h>
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
        draw_background();
        draw_history_lines();
        draw_barriers();
        if (m_game.start_pos().has_value()) {
            draw_start_circle();
        }
        if (m_game.current_pos().has_value()) {
            draw_current_pos_circle();
        }
        draw_ui();
        EndDrawing();
    }

private:
    struct BoardSizes {
        Vector2 offset;
        float grid_square;
        float square_padding;
        float inner_square;
        Rectangle board_rect;
    };

    void draw_background_square(const int x, const int y) const
    {
        const Vector2i rect_pos { static_cast<int>(
                                      m_board_sizes.offset.x + m_board_sizes.square_padding
                                      + static_cast<float>(x) * m_board_sizes.grid_square),
                                  static_cast<int>(
                                      m_board_sizes.offset.y + m_board_sizes.square_padding
                                      + static_cast<float>(y) * m_board_sizes.grid_square) };
        const int rect_size = static_cast<int>(m_board_sizes.inner_square);
        DrawRectangle(rect_pos.x, rect_pos.y, rect_size, rect_size, raylib::Color(168, 168, 168));
    }

    void draw_filled_circle(const int x, const int y) const
    {
        const Vector2i circle_pos {
            static_cast<int>(
                m_board_sizes.offset.x + m_board_sizes.grid_square / 2
                + static_cast<float>(x) * m_board_sizes.grid_square),
            static_cast<int>(
                m_board_sizes.offset.y + m_board_sizes.grid_square / 2
                + static_cast<float>(y) * m_board_sizes.grid_square)
        };
        const float circle_radius = m_board_sizes.inner_square / 2;
        const Color circle_color = m_game.result().has_value()
            ? m_game.result().value() == FullBoardGame::Result::won ? DARKGREEN : RED
            : GRAY;
        DrawCircle(circle_pos.x, circle_pos.y, circle_radius, circle_color);
    }

    void draw_background() const
    {
        for (int x = 0; x < m_game.size(); ++x) {
            for (int y = 0; y < m_game.size(); ++y) {
                draw_background_square(x, y);
                if (m_game.filled_at({ x, y })) {
                    draw_filled_circle(x, y);
                }
            }
        }
    }

    void draw_history_lines() const
    {
        for (auto [dir, from, to] : m_game.move_history()) {
            const Vector2 start { m_board_sizes.offset.x + m_board_sizes.grid_square / 2.0f
                                      + static_cast<float>(from.x) * m_board_sizes.grid_square,
                                  m_board_sizes.offset.y + m_board_sizes.grid_square / 2.0f
                                      + static_cast<float>(from.y) * m_board_sizes.grid_square };
            const Vector2 end { m_board_sizes.offset.x + m_board_sizes.grid_square / 2.0f
                                    + static_cast<float>(to.x) * m_board_sizes.grid_square,
                                m_board_sizes.offset.y + m_board_sizes.grid_square / 2.0f
                                    + static_cast<float>(to.y) * m_board_sizes.grid_square };
            DrawLineEx(start, end, 5.0f, BLUE);
        }
    }

    void draw_barriers() const
    {
        for (const auto [x, y] : m_game.barrier_positions()) {
            const Vector2i circle_pos {
                static_cast<int>(
                    m_board_sizes.offset.x + m_board_sizes.grid_square / 2
                    + static_cast<float>(x) * m_board_sizes.grid_square),
                static_cast<int>(
                    m_board_sizes.offset.y + m_board_sizes.grid_square / 2
                    + static_cast<float>(y) * m_board_sizes.grid_square)
            };
            const float circle_radius = m_board_sizes.inner_square / 2;
            DrawCircle(circle_pos.x, circle_pos.y, circle_radius, BLACK);
        }
    }

    void draw_start_circle() const
    {
        const Vector2i circle_pos {
            static_cast<int>(
                m_board_sizes.offset.x + m_board_sizes.grid_square / 2
                + static_cast<float>(m_game.start_pos()->x) * m_board_sizes.grid_square),
            static_cast<int>(
                m_board_sizes.offset.y + m_board_sizes.grid_square / 2
                + static_cast<float>(m_game.start_pos()->y) * m_board_sizes.grid_square)
        };
        const float circle_radius = m_board_sizes.inner_square / 4;
        DrawCircle(circle_pos.x, circle_pos.y, circle_radius, BLUE);
    }

    void draw_current_pos_circle() const
    {
        const Vector2i circle_pos {
            static_cast<int>(
                m_board_sizes.offset.x + m_board_sizes.grid_square / 2
                + static_cast<float>(m_game.current_pos()->x) * m_board_sizes.grid_square),
            static_cast<int>(
                m_board_sizes.offset.y + m_board_sizes.grid_square / 2
                + static_cast<float>(m_game.current_pos()->y) * m_board_sizes.grid_square)
        };
        const float circle_radius = m_board_sizes.inner_square / 2;
        DrawCircle(circle_pos.x, circle_pos.y, circle_radius, BLUE);
    }

    [[nodiscard]] BoardSizes calc_board_sizes() const
    {
        const Vector2i screen_size { GetScreenWidth(), GetScreenHeight() };
        const auto min_size = static_cast<float>(std::min(screen_size.x, std::max(1, screen_size.y - 100)));
        BoardSizes sizes {};
        sizes.offset.x = (static_cast<float>(screen_size.x) - min_size) / 2.0f;
        sizes.offset.y = 100.0f;
        sizes.grid_square = min_size / static_cast<float>(m_game.size());
        sizes.square_padding = 0.05f * sizes.grid_square;
        sizes.inner_square = min_size / static_cast<float>(m_game.size()) - 2 * sizes.square_padding;
        sizes.board_rect = Rectangle { sizes.offset.x, sizes.offset.y, min_size, min_size };
        return sizes;
    }

    [[nodiscard]] std::optional<Vector2i> mouse_to_grid() const
    {
        const raylib::Vector2 mouse_pos = GetMousePosition();
        if (!CheckCollisionPointRec(mouse_pos, m_board_sizes.board_rect)) {
            return std::nullopt;
        }
        if (const Vector2i grid_pos
            = { static_cast<int>((mouse_pos.x - m_board_sizes.offset.x) / m_board_sizes.grid_square),
                static_cast<int>((mouse_pos.y - m_board_sizes.offset.y) / m_board_sizes.grid_square) };
            m_game.in_bounds(grid_pos)) {
            return grid_pos;
        }
        return std::nullopt;
    }

    void handle_click()
    {
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

    void draw_ui() const
    {
        GuiButton({ 10.0f, 10.0f, 50.0f, 20.0f }, "Restart");
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
            handle_click();
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

#include <array>
#include <optional>

#include <raylib-cpp.hpp>

namespace rl = raylib;

enum class Direction { north, east, south, west };
enum class BoardState { empty, filled };

static int dir_idx(const Direction dir)
{
    switch (dir) {
    case Direction::north:
        return 0;
    case Direction::east:
        return 1;
    case Direction::south:
        return 2;
    case Direction::west:
        return 3;
    default:
        return 0;
    }
}

static Direction idx_dir(const int idx)
{
    switch (idx) {
    case 0:
        return Direction::north;
    case 1:
        return Direction::east;
    case 2:
        return Direction::south;
    case 3:
        return Direction::west;
    default:
        return Direction::north;
    }
}

struct Vector2i {
    int x;
    int y;

    bool operator==(const Vector2i& other) const
    {
        return x == other.x && y == other.y;
    }
};

[[maybe_unused]] static std::optional<Direction> next_dir(const Direction dir)
{
    switch (dir) {
    case Direction::north:
        return Direction::east;
    case Direction::east:
        return Direction::south;
    case Direction::south:
        return Direction::west;
    default:
        return {};
    }
}

constexpr int c_window_width = 800;
constexpr int c_window_height = 800;
constexpr int c_board_size = 6;

static int pos_to_idx(const Vector2i pos)
{
    return pos.y * c_board_size + pos.x;
}

[[maybe_unused]] static Vector2i idx_to_pos(const size_t idx)
{
    const int y = static_cast<int>(idx / c_board_size);
    const int x = static_cast<int>(idx % c_board_size);
    return { x, y };
}

static bool in_bounds(const Vector2i pos)
{
    return pos.x >= 0 && pos.x < c_board_size && pos.y >= 0 && pos.y < c_board_size;
}

enum class GameResult { won, lost };

struct MoveRecord {
    Direction dir;
    Vector2i from;
    Vector2i to;
};

struct MoveResult {
    std::optional<MoveRecord> record {};
    std::optional<GameResult> game_result {};
};

static void undo_move(const MoveRecord& record, Vector2i& current_pos, std::vector<BoardState>& board_state)
{
    if (record.to != current_pos) [[unlikely]] {
        TraceLog(LOG_ERROR, "Invalid undo");
        return;
    }
    const int min_x = std::min(record.from.x, record.to.x);
    const int max_x = std::max(record.from.x, record.to.x);
    const int min_y = std::min(record.from.y, record.to.y);
    const int max_y = std::max(record.from.y, record.to.y);
    for (int x = min_x; x < max_x + 1; ++x) {
        for (int y = min_y; y < max_y + 1; ++y) {
            board_state[pos_to_idx({ x, y })] = BoardState::empty;
        }
    }
    board_state[pos_to_idx(record.from)] = BoardState::filled;
    current_pos = record.from;
}

static MoveResult move(const Direction dir, Vector2i& current_pos, std::vector<BoardState>& board_state)
{
    const Vector2i start = current_pos;
    MoveResult result;
    bool moved = false;
    const auto inc_y = dir == Direction::north || dir == Direction::south ? dir == Direction::north ? -1 : 1 : 0;
    const auto inc_x = dir == Direction::east || dir == Direction::west ? dir == Direction::east ? 1 : -1 : 0;
    while (in_bounds({ current_pos.x + inc_x, current_pos.y + inc_y })
           && board_state[pos_to_idx({ current_pos.x + inc_x, current_pos.y + inc_y })] == BoardState::empty) {
        current_pos = { current_pos.x + inc_x, current_pos.y + inc_y };
        board_state[pos_to_idx({ current_pos.x, current_pos.y })] = BoardState::filled;
        moved = true;
    }
    if (moved) {
        result.record = MoveRecord { .dir = dir, .from = start, .to = current_pos };

        bool full = true;
        for (int i = 0; i < c_board_size * c_board_size; ++i) {
            if (board_state[i] == BoardState::empty) {
                full = false;
                break;
            }
        }
        if (full) {
            result.game_result = GameResult::won;
            return result;
        }

        bool trapped = true;
        for (std::array<Vector2i, 4> neighbor_offsets { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
             const auto [off_x, off_y] : neighbor_offsets) {
            if (const Vector2i neighbor_pos { current_pos.x + off_x, current_pos.y + off_y };
                in_bounds(neighbor_pos) && board_state[pos_to_idx(neighbor_pos)] == BoardState::empty) {
                trapped = false;
                break;
            }
        }
        if (trapped) {
            result.game_result = GameResult::lost;
            return result;
        }
    }
    return result;
}

static std::optional<GameResult> solve_step(
    Vector2i& current_pos, std::vector<MoveRecord>& history, std::vector<BoardState>& board_state)
{
    auto undo = [&] {
        if (history.empty()) {
            return false;
        }
        undo_move(history[history.size() - 1], current_pos, board_state);
        history.pop_back();
        return true;
    };

    std::optional<GameResult> result;
    auto try_move = [&](const Direction dir) -> bool {
        const auto [record, game_result] = move(dir, current_pos, board_state);
        result = game_result;
        if (record.has_value()) {
            history.push_back(*record);
            return true;
        }
        return false;
    };

    auto start = Direction::north;
    while (true) {
        for (int i = dir_idx(start); i < 4; ++i) {
            if (try_move(idx_dir(i))) {
                return result;
            }
        }
        std::optional<Direction> next;
        do {
            const Direction last = history[history.size() - 1].dir;
            if (!undo()) {
                return result;
            }
            next = next_dir(last);
        } while (!next.has_value());
        start = *next;
    }
}

enum class GameState { manual, solving };

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    const rl::Window window(c_window_width, c_window_height, "Full Board Solver");

    constexpr int square_padding = 10;
    constexpr int square_size_px = c_window_width / c_board_size - 2 * square_padding;
    constexpr int grid_square_size = c_window_width / c_board_size;
    std::optional<Vector2i> start_pos;
    std::optional<Vector2i> current_pos;
    std::vector<MoveRecord> move_history;
    std::vector board_state(c_board_size * c_board_size, BoardState::empty);
    std::vector<Vector2i> barriers;
    std::optional<GameResult> result;
    auto state = GameState::manual;

    const auto reset = [&] {
        start_pos.reset();
        current_pos.reset();
        move_history.clear();
        result.reset();
        board_state = std::vector(c_board_size * c_board_size, BoardState::empty);
        for (auto [x, y] : barriers) {
            board_state[pos_to_idx({ x, y })] = BoardState::filled;
        }
    };

    while (!window.ShouldClose()) {

        if (state == GameState::manual) {
            if (IsKeyPressed(KEY_R)) {
                reset();
            }

            if (IsKeyPressed(KEY_U) && !move_history.empty() && current_pos.has_value()) {
                undo_move(move_history[move_history.size() - 1], *current_pos, board_state);
                move_history.pop_back();
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !result.has_value()) {
                const rl::Vector2 mouse_pos = GetMousePosition();
                const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / grid_square_size),
                                            static_cast<int>(mouse_pos.y / grid_square_size) };
                if (current_pos.has_value()) {
                    if (grid_pos.x == current_pos->x && grid_pos.y != current_pos->y) {
                        auto [record, game_result] = move(
                            grid_pos.y > current_pos->y ? Direction::south : Direction::north,
                            *current_pos,
                            board_state);
                        if (record.has_value()) {
                            move_history.push_back(*record);
                        }
                        result = game_result;
                    }
                    else if (grid_pos.x != current_pos->x && grid_pos.y == current_pos->y) {
                        auto [record, game_result] = move(
                            grid_pos.x > current_pos->x ? Direction::east : Direction::west, *current_pos, board_state);
                        if (record.has_value()) {
                            move_history.push_back(*record);
                        }
                        result = game_result;
                    }
                }
                else {
                    start_pos = grid_pos;
                    current_pos = grid_pos;
                    board_state[pos_to_idx(grid_pos)] = BoardState::filled;
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !start_pos.has_value()) {
                const rl::Vector2 mouse_pos = GetMousePosition();
                if (const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / grid_square_size),
                                                static_cast<int>(mouse_pos.y / grid_square_size) };
                    std::ranges::find(barriers, grid_pos) == barriers.end()) {
                    barriers.push_back(grid_pos);
                    board_state[pos_to_idx(grid_pos)] = BoardState::filled;
                }
                else {
                    std::erase(barriers, grid_pos);
                    board_state[pos_to_idx(grid_pos)] = BoardState::empty;
                }
            }
            if (IsKeyPressed(KEY_S) && current_pos.has_value()) {
                result = solve_step(*current_pos, move_history, board_state);
            }
            if (IsKeyPressed(KEY_A)) {
                state = GameState::solving;
            }
        }
        else {
            auto next_pos = [&](const Vector2i prev) {
                int i = pos_to_idx(prev);
                while (i < c_board_size * c_board_size) {
                    i++;
                    if (std::ranges::find(barriers, idx_to_pos(i)) == barriers.end()) {
                        break;
                    }
                }
                return idx_to_pos(i);
            };
            if (!start_pos.has_value()) {
                const Vector2i next = next_pos(idx_to_pos(-1));
                start_pos = next;
                current_pos = next;
                board_state[pos_to_idx(next)] = BoardState::filled;
            }
            result = solve_step(*current_pos, move_history, board_state);
            if (move_history.empty()) {
                int i = pos_to_idx(start_pos.value());
                reset();
                if (i >= c_board_size * c_board_size) {
                    state = GameState::manual;
                }
                else {
                    const Vector2i next = next_pos(*start_pos);
                    start_pos = next;
                    current_pos = next;
                    board_state[pos_to_idx(next)] = BoardState::filled;
                }
            }
            if (IsKeyPressed(KEY_A) || (result.has_value() && result.value() == GameResult::won)) {
                state = GameState::manual;
            }
        }

        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        for (int x = 0; x < c_board_size; ++x) {
            for (int y = 0; y < c_board_size; ++y) {
                DrawRectangle(
                    square_padding + x * grid_square_size,
                    square_padding + y * grid_square_size,
                    square_size_px,
                    square_size_px,
                    rl::Color(168, 168, 168));
                if (board_state[pos_to_idx({ x, y })] == BoardState::filled) {
                    DrawCircle(
                        grid_square_size / 2 + x * grid_square_size,
                        grid_square_size / 2 + y * grid_square_size,
                        static_cast<float>(square_size_px) / 2,
                        result.has_value() ? result.value() == GameResult::won ? DARKGREEN : RED : GRAY);
                }
            }
        }
        for (auto [dir, from, to] : move_history) {
            DrawLineEx(
                { grid_square_size / 2.0f + static_cast<float>(from.x) * grid_square_size,
                  grid_square_size / 2.0f + static_cast<float>(from.y) * grid_square_size },
                { grid_square_size / 2.0f + static_cast<float>(to.x) * grid_square_size,
                  grid_square_size / 2.0f + static_cast<float>(to.y) * grid_square_size },
                5.0f,
                BLUE);
        }
        for (const auto [x, y] : barriers) {
            DrawCircle(
                grid_square_size / 2 + x * grid_square_size,
                grid_square_size / 2 + y * grid_square_size,
                static_cast<float>(square_size_px) / 2,
                BLACK);
        }
        if (start_pos.has_value()) {
            DrawCircle(
                grid_square_size / 2 + start_pos->x * grid_square_size,
                grid_square_size / 2 + start_pos->y * grid_square_size,
                static_cast<float>(square_size_px) / 4,
                BLUE);
        }
        if (current_pos.has_value()) {
            DrawCircle(
                grid_square_size / 2 + current_pos->x * grid_square_size,
                grid_square_size / 2 + current_pos->y * grid_square_size,
                static_cast<float>(square_size_px) / 2,
                BLUE);
        }
        EndDrawing();
    }
}
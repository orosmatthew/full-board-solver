#include <optional>
#include <raylib-cpp.hpp>

namespace rl = raylib;

enum class Direction { north, east, south, west };
enum class BoardState { empty, filled };

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
    case Direction::west:
    default:
        return {};
    }
}

constexpr int c_window_width = 800;
constexpr int c_window_height = 800;
constexpr int c_board_size = 6;

static size_t pos_to_idx(const Vector2i pos)
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

struct MoveResult {
    bool moved;
    Vector2i new_pos;
    std::vector<BoardState> new_state;
    std::optional<GameResult> game_result {};
};

MoveResult move(const Direction dir, const Vector2i current_pos, const std::vector<BoardState>& board_stat)
{
    MoveResult result { .moved = false, .new_pos = current_pos, .new_state = board_stat };
    const auto inc_y = dir == Direction::north || dir == Direction::south ? dir == Direction::north ? 1 : -1 : 0;
    const auto inc_x = dir == Direction::east || dir == Direction::west ? dir == Direction::east ? 1 : -1 : 0;
    while (in_bounds({ current_pos.x + inc_x, current_pos.y + inc_y })
           && result.new_state[pos_to_idx({ current_pos.x + inc_x, current_pos.y + inc_y })] == BoardState::empty) {
        result.new_pos = { current_pos.x + inc_x, current_pos.y + inc_y };
        result.new_state[pos_to_idx({ current_pos.x, current_pos.y })] = BoardState::filled;
        result.moved = true;
    }
    if (result.moved) {
        bool full = true;
        for (int i = 0; i < c_board_size * c_board_size; ++i) {
            if (result.new_state[i] == BoardState::empty) {
                full = false;
                break;
            }
        }
        if (full) {
            result.game_result = GameResult::won;
            result.new_pos = current_pos;
            return result;
        }

        bool trapped = true;
        for (std::array<Vector2i, 4> neighbor_offsets { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
             const auto [off_x, off_y] : neighbor_offsets) {
            if (const Vector2i neighbor_pos { current_pos.x + off_x, current_pos.y + off_y };
                in_bounds(neighbor_pos) && result.new_state[pos_to_idx(neighbor_pos)] == BoardState::empty) {
                trapped = false;
                break;
            }
        }
        if (trapped) {
            result.game_result = GameResult::lost;
            result.new_pos = current_pos;
            return result;
        }
    }
    result.new_pos = current_pos;
    return result;
}

int main()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    const rl::Window window(c_window_width, c_window_height, "Full Board Solver");

    constexpr int square_padding = 10;
    constexpr int square_size_px = c_window_width / c_board_size - 2 * square_padding;
    constexpr int grid_square_size = c_window_width / c_board_size;
    std::optional<Vector2i> start_pos;
    std::optional<Vector2i> current_pos;
    std::vector<Direction> move_history;
    std::vector board_state(c_board_size * c_board_size, BoardState::empty);
    std::vector<Vector2i> barriers;

    const auto reset = [&] {
        start_pos.reset();
        current_pos.reset();
        board_state = std::vector(c_board_size * c_board_size, BoardState::empty);
        for (auto [x, y] : barriers) {
            board_state[pos_to_idx({ x, y })] = BoardState::filled;
        }
    };

    while (!window.ShouldClose()) {

        auto move = [&](const Direction dir) -> bool {
            const auto inc_y
                = dir == Direction::north || dir == Direction::south ? dir == Direction::north ? 1 : -1 : 0;
            const auto inc_x = dir == Direction::east || dir == Direction::west ? dir == Direction::east ? 1 : -1 : 0;
            bool moved = false;
            while (in_bounds({ current_pos->x + inc_x, current_pos->y + inc_y })
                   && board_state[pos_to_idx({ current_pos->x + inc_x, current_pos->y + inc_y })]
                       == BoardState::empty) {
                current_pos = { current_pos->x + inc_x, current_pos->y + inc_y };
                board_state[pos_to_idx({ current_pos->x, current_pos->y })] = BoardState::filled;
                moved = true;
            }
            if (moved) {
                move_history.push_back(dir);

                bool trapped = true;
                for (std::array<Vector2i, 4> neighbor_offsets { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
                     const auto [off_x, off_y] : neighbor_offsets) {
                    if (const Vector2i neighbor_pos { current_pos->x + off_x, current_pos->y + off_y };
                        in_bounds(neighbor_pos) && board_state[pos_to_idx(neighbor_pos)] == BoardState::empty) {
                        trapped = false;
                        break;
                    }
                }
                if (trapped) {
                    return false;
                }

                bool full = true;
                for (int i = 0; i < c_board_size * c_board_size; ++i) {
                    if (board_state[i] == BoardState::empty) {
                        full = false;
                        break;
                    }
                }
                if (full) {
                    return false;
                }
            }
            return true;
        };

        if (IsKeyPressed(KEY_R)) {
            reset();
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const rl::Vector2 mouse_pos = GetMousePosition();
            const Vector2i grid_pos = { static_cast<int>(mouse_pos.x / grid_square_size),
                                        static_cast<int>(mouse_pos.y / grid_square_size) };
            if (current_pos.has_value()) {
                if (grid_pos.x == current_pos->x && grid_pos.y != current_pos->y) {
                    if (!move(grid_pos.y > current_pos->y ? Direction::north : Direction::south)) {
                        reset();
                    }
                }
                else if (grid_pos.x != current_pos->x && grid_pos.y == current_pos->y) {
                    if (!move(grid_pos.x > current_pos->x ? Direction::east : Direction::west)) {
                        reset();
                    }
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
                        static_cast<float>(square_size_px) / 4,
                        GRAY);
                }
            }
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
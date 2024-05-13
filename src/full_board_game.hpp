#pragma once

#include "common.hpp"

class FullBoardGame {
    enum class BoardState { empty, filled };

public:
    enum class Result { won, lost };

    struct MoveRecord {
        Direction dir;
        Vector2i from;
        Vector2i to;
    };

    struct MoveResult {
        std::optional<MoveRecord> record {};
        std::optional<Result> game_result {};
    };

    explicit FullBoardGame(const int size)
        : m_size(size)
        , m_state(m_size * m_size, BoardState::empty)
    {
    }

    [[nodiscard]] int pos_to_idx(const Vector2i pos) const
    {
        return pos.y * m_size + pos.x;
    }

    [[nodiscard]] Vector2i idx_to_pos(const size_t idx) const
    {
        const int y = static_cast<int>(idx / m_size);
        const int x = static_cast<int>(idx % m_size);
        return { x, y };
    }

    [[nodiscard]] bool in_bounds(const Vector2i pos) const
    {
        return pos.x >= 0 && pos.x < m_size && pos.y >= 0 && pos.y < m_size;
    }

    [[nodiscard]] bool filled_at(const Vector2i pos) const
    {
        return m_state[pos_to_idx(pos)] == BoardState::filled;
    }

    bool undo()
    {
        if (m_history.empty() || !m_current_pos.has_value()) {
            return false;
        }
        const auto [dir, from, to] = m_history[m_history.size() - 1];
        const int min_x = std::min(from.x, to.x);
        const int max_x = std::max(from.x, to.x);
        const int min_y = std::min(from.y, to.y);
        const int max_y = std::max(from.y, to.y);
        for (int x = min_x; x < max_x + 1; ++x) {
            for (int y = min_y; y < max_y + 1; ++y) {
                m_state[pos_to_idx({ x, y })] = BoardState::empty;
            }
        }
        m_state[pos_to_idx(from)] = BoardState::filled;
        m_current_pos = from;
        m_history.pop_back();
        return true;
    }

    [[nodiscard]] std::optional<Result> result() const
    {
        return m_result;
    }

    [[nodiscard]] bool won() const
    {
        return m_result.has_value() && m_result.value() == Result::won;
    }

    [[nodiscard]] std::optional<Vector2i> current_pos() const
    {
        return m_current_pos;
    }

    [[nodiscard]] std::optional<Vector2i> start_pos() const
    {
        return m_start_pos;
    }

    void set_start(Vector2i pos)
    {
        if (m_history.empty() && !is_barrier(pos)) {
            m_start_pos = pos;
            m_current_pos = pos;
            m_state[pos_to_idx(pos)] = BoardState::filled;
        }
    }

    [[nodiscard]] std::optional<MoveRecord> last_move() const
    {
        if (m_history.empty()) {
            return {};
        }
        return m_history.at(m_history.size() - 1);
    }

    MoveResult move(const Direction dir)
    {
        if (!m_current_pos.has_value()) {
            return MoveResult {};
        }
        const Vector2i start = *m_current_pos;
        MoveResult result;
        bool moved = false;
        const auto inc_y = dir == Direction::north || dir == Direction::south ? dir == Direction::north ? -1 : 1 : 0;
        const auto inc_x = dir == Direction::east || dir == Direction::west ? dir == Direction::east ? 1 : -1 : 0;
        while (in_bounds({ m_current_pos->x + inc_x, m_current_pos->y + inc_y })
               && m_state[pos_to_idx({ m_current_pos->x + inc_x, m_current_pos->y + inc_y })] == BoardState::empty) {
            m_current_pos = { m_current_pos->x + inc_x, m_current_pos->y + inc_y };
            m_state[pos_to_idx({ m_current_pos->x, m_current_pos->y })] = BoardState::filled;
            moved = true;
        }
        if (moved) {
            result.record = MoveRecord { .dir = dir, .from = start, .to = *m_current_pos };
            m_history.push_back(*result.record);
            m_result.reset();
            m_result = check_game_result();
            result.game_result = m_result;
        }
        return result;
    }

    void set_barrier(const Vector2i pos, const bool value)
    {
        const int idx = pos_to_idx(pos);
        if (value) {
            if (std::ranges::find(m_barriers, pos) == m_barriers.end() && m_state[idx] == BoardState::empty) {
                m_barriers.push_back(pos);
                m_state[pos_to_idx(pos)] = BoardState::filled;
            }
        }
        else if (std::ranges::find(m_barriers, pos) != m_barriers.end()) {
            m_state[pos_to_idx(pos)] = BoardState::empty;
            std::erase(m_barriers, pos);
        }
        m_result = check_game_result();
    }

    [[nodiscard]] bool is_barrier(const Vector2i pos) const
    {
        return std::ranges::find(m_barriers, pos) != m_barriers.end();
    }

    void toggle_barrier(const Vector2i pos)
    {
        set_barrier(pos, !is_barrier(pos));
    }

    void reset()
    {
        m_start_pos.reset();
        m_current_pos.reset();
        m_history.clear();
        m_result.reset();
        m_state.clear();
        m_state.resize(m_size * m_size, BoardState::empty);
        m_barriers.clear();
    }

    void reset_leave_barriers()
    {
        m_start_pos.reset();
        m_current_pos.reset();
        m_history.clear();
        m_result.reset();
        m_state.clear();
        m_state.resize(m_size * m_size, BoardState::empty);
        for (auto [x, y] : m_barriers) {
            m_state[pos_to_idx({ x, y })] = BoardState::filled;
        }
    }

    [[nodiscard]] int size() const
    {
        return m_size;
    }

    [[nodiscard]] const std::vector<MoveRecord>& move_history() const
    {
        return m_history;
    }

    [[nodiscard]] const std::vector<Vector2i>& barrier_positions() const
    {
        return m_barriers;
    }

    [[nodiscard]] std::optional<Result> check_game_result() const
    {
        bool full = true;
        for (int i = 0; i < m_size * m_size; ++i) {
            if (m_state[i] == BoardState::empty) {
                full = false;
                break;
            }
        }
        if (full) {
            return Result::won;
        }

        bool trapped = true;
        for (std::array<Vector2i, 4> neighbor_offsets { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
             const auto [off_x, off_y] : neighbor_offsets) {
            if (const Vector2i neighbor_pos { m_current_pos->x + off_x, m_current_pos->y + off_y };
                in_bounds(neighbor_pos) && m_state[pos_to_idx(neighbor_pos)] == BoardState::empty) {
                trapped = false;
                break;
            }
        }
        if (trapped) {
            return Result::lost;
        }
        return std::nullopt;
    }

private:
    int m_size;
    std::optional<Vector2i> m_start_pos;
    std::optional<Vector2i> m_current_pos;
    std::vector<MoveRecord> m_history;
    std::vector<BoardState> m_state;
    std::vector<Vector2i> m_barriers;
    std::optional<Result> m_result;
};
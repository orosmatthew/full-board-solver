#pragma once

#include <optional>

enum class Direction { north, east, south, west };

inline int dir_idx(const Direction dir)
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

inline Direction idx_dir(const int idx)
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

inline std::optional<Direction> next_dir(const Direction dir)
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
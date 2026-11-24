#pragma once

#include "Tile.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>

/// Each action keeps only the original state of cells that changed.
class EditHistory
{
public:
    /// Capture column order before an action that can erase a base.
    void beginErase(const std::vector<Tile>& tiles);
    /// Call before changing this cell. Repeated calls in the same action are ignored.
    void remember(const std::vector<Tile>& tiles, sf::Vector2i cell);
    void finish(const std::vector<Tile>& tiles);
    bool undo(std::vector<Tile>& tiles);
    void clear();

private:
    struct CellBefore
    {
        std::optional<Tile> tile;
        std::size_t index = 0;
        bool presentAfter = false;
    };

    using CellKey = std::uint64_t;
    using Changes = std::unordered_map<CellKey, CellBefore>;

    struct Action
    {
        Changes cells;
        std::size_t bytes;
    };

    std::unordered_map<CellKey, std::size_t> orderBeforeErase;
    Changes pending;
    std::deque<Action> completed;
    std::size_t savedBytes = 0;
};

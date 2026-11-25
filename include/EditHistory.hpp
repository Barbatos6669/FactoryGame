#pragma once

#include "Tile.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>

/// Each action stores the changed columns on both sides of one edit.
class EditHistory
{
public:
    /// Capture column order before an action that can erase a base.
    void beginErase(const std::vector<Tile>& tiles);
    /// Call before changing this cell. Repeated calls in the same action are ignored.
    void remember(const std::vector<Tile>& tiles, sf::Vector2i cell);
    /// Capture a known column by index before a bulk edit, once per column.
    void rememberColumn(const Tile& tile, std::size_t index);
    /// Capture the whole footprint before a new rectangular paint action.
    void rememberRectangle(const std::vector<Tile>& tiles, sf::IntRect bounds);
    void finish(const std::vector<Tile>& tiles);
    bool undo(std::vector<Tile>& tiles);
    bool redo(std::vector<Tile>& tiles);
    bool canUndo() const { return appliedActions > 0; }
    bool canRedo() const { return appliedActions < actions.size(); }
    void clear();

private:
    struct IndexedTile
    {
        std::size_t index;
        Tile tile;
    };
    using Columns = std::vector<IndexedTile>;

    struct Action
    {
        Columns before;
        Columns after;
        std::size_t bytes() const;
    };

    static void restore(std::vector<Tile>& tiles, const Columns& remove, const Columns& insert);
    using CellKey = std::uint64_t;
    std::unordered_map<CellKey, std::size_t> orderBeforeErase;
    std::unordered_map<CellKey, std::optional<IndexedTile>> pending;
    std::deque<Action> actions;
    std::size_t appliedActions = 0;
    std::size_t savedBytes = 0;
};

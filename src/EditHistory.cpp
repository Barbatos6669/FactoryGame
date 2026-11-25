#include "EditHistory.hpp"

#include <algorithm>
#include <utility>

namespace
{
constexpr std::size_t undoActions = 32;
constexpr std::size_t undoBytes = 32 * 1024 * 1024;

std::uint64_t cellKey(sf::Vector2i cell)
{
    const std::uint64_t x = static_cast<std::uint32_t>(cell.x);
    const std::uint64_t y = static_cast<std::uint32_t>(cell.y);
    return (x << 32) | y;
}

std::size_t layerBytes(const Tile& tile)
{
    std::size_t bytes = tile.layers.capacity() * sizeof(TerrainLayer);
    for (const TerrainLayer& layer : tile.layers)
        bytes += layer.asset.id.capacity() + layer.groundAsset.id.capacity() + 2;
    return bytes;
}
}

void EditHistory::beginErase(const std::vector<Tile>& tiles)
{
    orderBeforeErase.reserve(tiles.size());
    for (std::size_t index = 0; index < tiles.size(); ++index)
        orderBeforeErase.emplace(cellKey(tiles[index].cell), index);
}

void EditHistory::remember(const std::vector<Tile>& tiles, sf::Vector2i cell)
{
    const auto key = cellKey(cell);
    auto [entry, inserted] = pending.try_emplace(key);
    if (!inserted)
        return;

    if (const Tile* tile = findTile(tiles, cell))
    {
        const auto initial = orderBeforeErase.find(key);
        const auto index = initial != orderBeforeErase.end() ? initial->second :
            static_cast<std::size_t>(tile - tiles.data());
        entry->second = IndexedTile{index, *tile};
    }
}

void EditHistory::rememberColumn(const Tile& tile, std::size_t index)
{
    pending[cellKey(tile.cell)] = IndexedTile{index, tile};
}

void EditHistory::rememberEmpty(sf::Vector2i cell)
{
    pending.try_emplace(cellKey(cell));
}

void EditHistory::rememberRectangle(const std::vector<Tile>& tiles, sf::IntRect bounds)
{
    const auto end = bounds.position + bounds.size;
    for (int y = bounds.position.y; y < end.y; ++y)
        for (int x = bounds.position.x; x < end.x; ++x)
            pending.try_emplace(cellKey({x, y}));

    // Unpainted cells keep an empty entry. Capture existing columns and their save order in one pass.
    for (std::size_t index = 0; index < tiles.size(); ++index)
    {
        const Tile& tile = tiles[index];
        if (bounds.contains(tile.cell))
            rememberColumn(tile, index);
    }
}

std::size_t EditHistory::Action::bytes() const
{
    std::size_t total = sizeof(Action) + (before.capacity() + after.capacity()) * sizeof(IndexedTile);
    for (const IndexedTile& column : before)
        total += layerBytes(column.tile);
    for (const IndexedTile& column : after)
        total += layerBytes(column.tile);
    return total;
}

void EditHistory::finish(const std::vector<Tile>& tiles)
{
    orderBeforeErase.clear();
    if (pending.empty())
        return;

    Action edit;
    for (std::size_t index = 0; index < tiles.size(); ++index)
    {
        const Tile& tile = tiles[index];
        const auto found = pending.find(cellKey(tile.cell));
        if (found == pending.end())
            continue;

        auto& before = found->second;
        if (!before || before->index != index || before->tile.layers != tile.layers)
        {
            edit.after.push_back({index, tile});
            if (before)
                edit.before.push_back(std::move(*before));
        }
        pending.erase(found);
    }

    // A remembered column missing from the final map was either erased or empty all along.
    for (auto& entry : pending)
        if (entry.second)
            edit.before.push_back(std::move(*entry.second));
    pending.clear();
    if (edit.before.empty() && edit.after.empty())
        return;

    std::sort(edit.before.begin(), edit.before.end(), [](const IndexedTile& a, const IndexedTile& b) {
        return a.index < b.index;
    });

    // A real new edit replaces the redo branch. A no-op leaves it available.
    while (canRedo())
    {
        savedBytes -= actions.back().bytes();
        actions.pop_back();
    }
    actions.push_back(std::move(edit));
    savedBytes += actions.back().bytes();
    while (actions.size() > 1 && (actions.size() > undoActions || savedBytes > undoBytes))
    {
        savedBytes -= actions.front().bytes();
        actions.pop_front();
    }
    appliedActions = actions.size();
}

void EditHistory::restore(std::vector<Tile>& tiles, const Columns& remove, const Columns& insert)
{
    // Both lists are in map order. Untouched columns keep their order in either direction.
    Columns replacements = insert;
    std::vector<Tile> restored;
    restored.reserve(tiles.size() - remove.size() + insert.size());
    auto nextInsert = replacements.begin();
    const auto insertColumns = [&] {
        while (nextInsert != replacements.end() && nextInsert->index == restored.size())
        {
            restored.push_back(std::move(nextInsert->tile));
            ++nextInsert;
        }
    };

    auto nextRemove = remove.begin();
    for (std::size_t index = 0; index < tiles.size(); ++index)
    {
        if (nextRemove != remove.end() && nextRemove->index == index)
        {
            ++nextRemove;
            continue;
        }
        insertColumns();
        restored.push_back(std::move(tiles[index]));
    }
    insertColumns();
    tiles.swap(restored);
}

bool EditHistory::undo(std::vector<Tile>& tiles)
{
    finish(tiles);
    if (!canUndo())
        return false;
    const auto& edit = actions[appliedActions - 1];
    restore(tiles, edit.after, edit.before);
    --appliedActions;
    return true;
}

bool EditHistory::redo(std::vector<Tile>& tiles)
{
    finish(tiles);
    if (!canRedo())
        return false;
    const auto& edit = actions[appliedActions];
    restore(tiles, edit.before, edit.after);
    ++appliedActions;
    return true;
}

void EditHistory::clear()
{
    orderBeforeErase.clear();
    pending.clear();
    actions.clear();
    appliedActions = 0;
    savedBytes = 0;
}

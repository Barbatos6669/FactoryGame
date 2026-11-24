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

bool samePlacement(const AssetPlacement& a, const AssetPlacement& b)
{
    return a.id == b.id && a.row == b.row && a.frame == b.frame;
}

bool sameTile(const Tile& a, const Tile& b)
{
    return std::equal(a.layers.begin(), a.layers.end(), b.layers.begin(), b.layers.end(),
        [](const TerrainLayer& left, const TerrainLayer& right) {
            return left.terrain == right.terrain && left.object == right.object &&
                   samePlacement(left.groundAsset, right.groundAsset) && samePlacement(left.asset, right.asset);
        });
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
    if (pending.find(key) != pending.end())
        return;

    CellBefore before;
    if (const Tile* tile = findTile(tiles, cell))
    {
        before.tile = *tile;
        const auto initial = orderBeforeErase.find(key);
        before.index = initial != orderBeforeErase.end() ? initial->second :
            static_cast<std::size_t>(tile - tiles.data());
    }
    pending.emplace(key, std::move(before));
}

void EditHistory::finish(const std::vector<Tile>& tiles)
{
    orderBeforeErase.clear();
    if (pending.empty())
        return;

    for (const Tile& tile : tiles)
    {
        const auto found = pending.find(cellKey(tile.cell));
        if (found == pending.end())
            continue;

        auto& before = found->second;
        if (before.tile && sameTile(*before.tile, tile))
            pending.erase(found);
        else
            before.presentAfter = true;
    }

    // An absent cell that stayed absent also made no change.
    for (auto found = pending.begin(); found != pending.end();)
    {
        if (!found->second.tile && !found->second.presentAfter)
            found = pending.erase(found);
        else
            ++found;
    }
    if (pending.empty())
        return;

    std::size_t bytes = sizeof(Action) + pending.bucket_count() * sizeof(void*);
    for (const auto& entry : pending)
    {
        bytes += sizeof(entry) + 2 * sizeof(void*);
        if (entry.second.tile)
            bytes += layerBytes(*entry.second.tile);
    }

    completed.push_back({std::move(pending), bytes});
    pending.clear();
    savedBytes += bytes;

    // An unusually large action is still undoable in one step.
    while (completed.size() > 1 && (completed.size() > undoActions || savedBytes > undoBytes))
    {
        savedBytes -= completed.front().bytes;
        completed.pop_front();
    }
}

bool EditHistory::undo(std::vector<Tile>& tiles)
{
    finish(tiles);
    if (completed.empty())
        return false;

    auto& action = completed.back();
    std::vector<CellBefore*> erased;
    for (auto& entry : action.cells)
    {
        auto& before = entry.second;
        if (before.tile && !before.presentAfter)
            erased.push_back(&before);
    }
    std::sort(erased.begin(), erased.end(), [](const CellBefore* a, const CellBefore* b) {
        return a->index < b->index;
    });

    // The remaining columns already have the right relative order.
    std::vector<Tile> restored;
    restored.reserve(tiles.size() + erased.size());
    auto nextErased = erased.begin();
    const auto restoreErased = [&] {
        while (nextErased != erased.end() && (*nextErased)->index == restored.size())
        {
            restored.push_back(std::move(*(*nextErased)->tile));
            ++nextErased;
        }
    };

    for (Tile& tile : tiles)
    {
        const auto found = action.cells.find(cellKey(tile.cell));
        if (found != action.cells.end() && !found->second.tile)
            continue;

        restoreErased();
        if (found != action.cells.end())
            restored.push_back(std::move(*found->second.tile));
        else
            restored.push_back(std::move(tile));
    }
    restoreErased();
    tiles.swap(restored);
    savedBytes -= action.bytes;
    completed.pop_back();
    return true;
}

void EditHistory::clear()
{
    orderBeforeErase.clear();
    pending.clear();
    completed.clear();
    savedBytes = 0;
}

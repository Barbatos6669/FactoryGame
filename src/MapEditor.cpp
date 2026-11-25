#include "MapEditor.hpp"

#include "AssetArt.hpp"
#include "Debug.hpp"
#include "Grid.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
bool isTerrainBrush(Brush brush)
{
    return brush == Brush::Grass || brush == Brush::Dirt || brush == Brush::Water;
}
}

MapEditor::MapEditor(std::vector<Tile> initialTiles, std::filesystem::path path)
    : savePath(std::move(path)), tiles(std::move(initialTiles))
{
}

void MapEditor::selectBrush(Brush choice)
{
    finishEdit();
    selectedAsset = {};
    brush = choice;
    if (isTerrainBrush(brush))
    {
        lastGround = brush;
        if (tool == TerrainTool::Erase)
            tool = TerrainTool::Paint;
    }
    else
        tool = TerrainTool::Paint;
}

void MapEditor::selectTool(TerrainTool choice, const AssetLibrary& library)
{
    finishEdit();
    tool = choice;
    if (tool == TerrainTool::Stack)
    {
        brush = lastGround;
        if (library.find(selectedAsset, true))
            return; // A ground stamp can be used on the raised block.
    }
    selectedAsset = {};
}

void MapEditor::selectStamp(AssetPlacement stamp)
{
    finishEdit();
    selectedAsset = std::move(stamp);
    tool = TerrainTool::Paint;
}

void MapEditor::cancelSelection()
{
    if (rectangle)
    {
        finishEdit();
        return;
    }
    finishEdit();
    selectedAsset = {};
    if (tool == TerrainTool::Erase)
        tool = TerrainTool::Paint;
}

void MapEditor::sampleCell(sf::Vector2i cell, bool groundOnly, EditorUI& ui, const AssetLibrary& library)
{
    const Tile* tile = findTile(tiles, cell);
    if (!tile || tile->layers.empty())
    {
        ui.notify("Nothing to pick here");
        return;
    }

    const TerrainLayer& surface = tile->layers.back();
    const bool pickGround = groundOnly || (surface.object == TileObject::None && surface.asset.id.empty());
    const AssetPlacement& stamp = pickGround ? surface.groundAsset : surface.asset;
    if (!stamp.id.empty())
    {
        const AssetEntry* entry = library.find(stamp, pickGround);
        if (!entry)
        {
            ui.notify(library.indexing() ? "Library is loading. Try picking again." : "This stamp is unavailable");
            return;
        }
        selectStamp(stamp);
        ui.notify("Picked " + entry->name);
        return;
    }

    const auto pickBrush = [&](Brush choice, const char* name) {
        selectBrush(choice);
        tool = TerrainTool::Paint;
        ui.notify(std::string("Picked ") + name);
    };
    if (pickGround)
    {
        switch (surface.terrain)
        {
            case TerrainType::Grass: pickBrush(Brush::Grass, "Grass"); break;
            case TerrainType::Dirt:  pickBrush(Brush::Dirt, "Dirt"); break;
            case TerrainType::Water: pickBrush(Brush::Water, "Water"); break;
        }
    }
    else if (surface.object == TileObject::Rock)
        pickBrush(Brush::Rock, "Rock");
    else if (surface.object == TileObject::Flower)
        pickBrush(Brush::Flower, "Flower");
}

std::size_t MapEditor::fillSurface(sf::Vector2i cell, const AssetLibrary& library)
{
    finishEdit();
    if (!paintsSurface(library))
        return 0;
    const auto patch = terrainFillCells(tiles, cell, brush, selectedAsset);
    if (patch.empty())
        return 0;

    for (const auto index : patch)
    {
        Tile& tile = tiles[index];
        history.rememberColumn(tile, index);
        paintGround(tile.layers.back(), brush, selectedAsset);
    }
    history.finish(tiles);
    return patch.size();
}

bool MapEditor::beginRectangle(sf::Vector2i cell, const sf::View& view, const AssetLibrary& library)
{
    if (!paintsSurface(library))
        return false;
    finishEdit();
    rectangle = RectanglePreview{cell, cell, view.getCenter(), view.getSize()};
    return true;
}

void MapEditor::updateRectangle(sf::Vector2i cell)
{
    if (rectangle)
        rectangle->end = cell;
}

std::optional<sf::IntRect> MapEditor::rectangleBounds() const
{
    if (!rectangle)
        return std::nullopt;
    const auto first = rectangle->anchor;
    const auto last = rectangle->end;
    return sf::IntRect{{std::min(first.x, last.x), std::min(first.y, last.y)},
                       {std::abs(last.x - first.x) + 1, std::abs(last.y - first.y) + 1}};
}

void MapEditor::commitRectangle(EditorUI& ui, AssetLibrary& library)
{
    const auto bounds = rectangleBounds();
    rectangle.reset();
    if (!bounds || !paintsSurface(library))
        return;

    history.rememberRectangle(tiles, *bounds);
    const bool painted = paintTerrainRectangle(tiles, *bounds, brush, selectedAsset);
    history.finish(tiles);
    if (painted)
        ui.notify("Painted " + std::to_string(bounds->size.x) + " x " + std::to_string(bounds->size.y));
    else
        ui.notify("Could not paint this terrain stamp");
}

MapEditor::TerrainStroke::TerrainStroke(sf::Vector2i start, sf::Vector2i cell, const sf::View& view)
    : pixel(start), lastCell(cell), viewCenter(view.getCenter()), viewSize(view.getSize()),
      visited{{cell.x, cell.y}}
{
}

void MapEditor::TerrainStroke::preparePicker(const Grid& grid, const std::vector<Tile>& tiles,
                                             const IsometricArt& art, AssetLibrary& library, bool erasing)
{
    if (picker)
        return;
    if (!erasing)
    {
        picker.emplace(grid, tiles, art, &library);
        return;
    }

    // The first click already erased a layer. Reconstruct the press-time scene once when it becomes a drag.
    auto& original = eraseSnapshot.emplace(tiles);
    if (firstErased)
    {
        auto found = std::find_if(original.begin(), original.end(), [&](const Tile& tile) {
            return tile.cell == firstErased->cell;
        });
        if (found == original.end())
            original.push_back(std::move(*firstErased));
        else
            *found = std::move(*firstErased);
        firstErased.reset();
    }
    picker.emplace(grid, original, art, &library);
}

void MapEditor::pressCell(sf::Vector2i cell, sf::Vector2i pixel, const sf::View& view,
                          EditorUI& ui, AssetLibrary& library)
{
    finishEdit();
    if (tool == TerrainTool::Erase || paintsSurface(library))
    {
        stroke.emplace(pixel, cell, view);
        if (tool == TerrainTool::Erase)
        {
            history.beginErase(tiles);
            if (const Tile* tile = findTile(tiles, cell))
                stroke->firstErased = *tile;
        }
    }
    editCell(cell, ui, library);
    if (!isDragging())
        history.finish(tiles);
}

void MapEditor::clearDecoration(sf::Vector2i cell)
{
    finishEdit();
    history.remember(tiles, cell);
    clearTileObject(tiles, cell);
    history.finish(tiles);
}

void MapEditor::editCell(sf::Vector2i cell, EditorUI& ui, AssetLibrary& library)
{
    history.remember(tiles, cell);
    if (tool == TerrainTool::Paint && !selectedAsset.id.empty())
    {
        const auto* entry = library.index().find(selectedAsset.id);
        if (entry && entry->placeable)
            placeAsset(tiles, cell, selectedAsset, entry->groundSurface);
        return;
    }
    switch (tool)
    {
        case TerrainTool::Paint:
            paintTile(tiles, cell, brush);
            break;
        case TerrainTool::Stack:
        {
            TerrainType ground = TerrainType::Grass;
            if (brush == Brush::Dirt) ground = TerrainType::Dirt;
            if (brush == Brush::Water) ground = TerrainType::Water;
            if (!stackTile(tiles, cell, ground))
                ui.notify("Height limit: " + std::to_string(MAX_TERRAIN_LAYERS) + " blocks");
            else if (!selectedAsset.id.empty())
                placeAsset(tiles, cell, selectedAsset, true);
            break;
        }
        case TerrainTool::Lower:
            if (!lowerTile(tiles, cell))
                ui.notify("No upper block to remove");
            break;
        case TerrainTool::Erase:
            eraseTile(tiles, cell);
            break;
    }
}

bool MapEditor::paintsSurface(const AssetLibrary& library) const
{
    if (tool != TerrainTool::Paint)
        return false;
    if (!selectedAsset.id.empty())
        return library.find(selectedAsset, true) != nullptr;
    return isTerrainBrush(brush);
}

void MapEditor::dragTo(sf::Vector2i end, const sf::RenderTarget& target, bool focused,
                       const Grid& grid, const Debug& debug, EditorUI& ui,
                       const IsometricArt& art, AssetLibrary& library)
{
    if (!stroke)
        return;
    TerrainStroke& active = *stroke;
    const auto& view = target.getView();
    if (!focused || view.getCenter() != active.viewCenter || view.getSize() != active.viewSize)
    {
        finishEdit();
        return;
    }
    if (active.pixel == end)
        return;
    const bool erasing = tool == TerrainTool::Erase;
    active.preparePicker(grid, tiles, art, library, erasing);
    std::set<std::pair<int, int>> cellsToErase;

    // SFML may report a long move in one event. Visit each pixel on that screen segment in order.
    const sf::Vector2i start = active.pixel;
    const double dx = static_cast<double>(end.x) - start.x;
    const double dy = static_cast<double>(end.y) - start.y;
    const auto steps = static_cast<long long>(std::max(std::abs(dx), std::abs(dy)));
    const auto size = target.getSize();
    bool crossedBoundary = false;
    for (long long step = 1; step <= steps; ++step)
    {
        const double t = static_cast<double>(step) / steps;
        const sf::Vector2i pixel{static_cast<int>(std::round(start.x + t * dx)),
                                 static_cast<int>(std::round(start.y + t * dy))};
        std::optional<sf::Vector2i> cell;
        if (insideWindow(pixel, size) && editorHit(ui, debug, size, pixel) == EditorCommand::Canvas)
            cell = active.picker->pick(projectedMousePoint(target, pixel));
        if (!cell)
        {
            crossedBoundary = true;
            break;
        }
        if (*cell == active.lastCell)
            continue;
        active.lastCell = *cell;
        if (!active.visited.emplace(cell->x, cell->y).second)
            continue;
        if (erasing)
            cellsToErase.emplace(cell->x, cell->y);
        else
        {
            const auto previousSize = tiles.size();
            editCell(*cell, ui, library);
            if (tiles.size() > previousSize)
                active.picker->includeTile(previousSize);
        }
    }
    if (erasing)
    {
        for (const auto& [x, y] : cellsToErase)
            history.remember(tiles, {x, y});
        eraseTiles(tiles, cellsToErase);
    }
    if (crossedBoundary)
        finishEdit();
    else
        active.pixel = end;
}

void MapEditor::finishEdit()
{
    rectangle.reset();
    // A release and an interrupted stroke both finish a single undoable map edit.
    if (!stroke)
        return;
    stroke.reset();
    history.finish(tiles);
}

void MapEditor::finishIfCameraMoved(const sf::View& view)
{
    if ((stroke && view.getCenter() != stroke->viewCenter) ||
        (rectangle && (view.getCenter() != rectangle->viewCenter || view.getSize() != rectangle->viewSize)))
        finishEdit();
}

bool MapEditor::undoEdit()
{
    rectangle.reset();
    stroke.reset();
    return history.undo(tiles);
}

bool MapEditor::redoEdit()
{
    rectangle.reset();
    stroke.reset();
    return history.redo(tiles);
}

bool MapEditor::loadMap()
{
    finishEdit();
    if (!loadTiles(tiles, savePath))
        return false;
    history.clear();
    return true;
}

// The diagnostics panel also covers the map. The palette has priority if the two overlap.
EditorCommand editorHit(const EditorUI& ui, const Debug& debug, sf::Vector2u size, sf::Vector2i pixel)
{
    const EditorCommand hit = ui.hitTest(size, pixel);
    if (hit == EditorCommand::Canvas && debug.panelBounds(size).contains(sf::Vector2f(pixel)))
        return EditorCommand::Panel;
    return hit;
}

sf::Vector2f projectedMousePoint(
    const sf::RenderTarget& target,
    sf::Vector2i pixel
)
{
    // SFML maps integer coordinates to a pixel corner. The rendered sample lies at its center.
    const auto first = target.mapPixelToCoords(pixel);
    const auto next = target.mapPixelToCoords(pixel + sf::Vector2i{1, 1});
    return (first + next) / 2.f;
}

bool insideWindow(
    sf::Vector2i pixel,
    sf::Vector2u size
)
{
    return pixel.x >= 0 && pixel.y >= 0 && static_cast<unsigned>(pixel.x) < size.x &&
           static_cast<unsigned>(pixel.y) < size.y;
}

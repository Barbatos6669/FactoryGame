#include "TerrainScene.hpp"
#include "AssetArt.hpp"
#include "Engineer.hpp"
#include "Grid.hpp"
#include "IsometricArt.hpp"
#include "Projection.hpp"
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>

namespace
{
// Ascending diagonals run from back to front. Within a diagonal, draw from left to right.
bool behind(sf::Vector2i a, sf::Vector2i b)
{
    const auto aDepth = static_cast<long long>(a.x) + a.y;
    const auto bDepth = static_cast<long long>(b.x) + b.y;
    return aDepth == bDepth ? a.x < b.x : aDepth < bDepth;
}

sf::Vector2f baseFront(sf::Vector2i cell, float side)
{
    return mapToView((sf::Vector2f(cell) + sf::Vector2f{1.f, 1.f}) * side);
}

float columnElevation(const Tile& tile, float side)
{
    return static_cast<float>(tile.layers.size() - 1) * side * terrainLayerHeightRatio;
}
}

float terrainElevationAt(const Grid& grid, const std::vector<Tile>& tiles, sf::Vector2f mapPosition)
{
    if (grid.contains(mapPosition))
    {
        if (const Tile* tile = findTile(tiles, grid.worldToCell(mapPosition)))
            return columnElevation(*tile, static_cast<float>(grid.cellSize));
    }
    return 0.f;
}

namespace
{
sf::FloatRect objectBounds(const sf::Image& image, sf::Vector2f top, float scale)
{
    return {top - objectAnchor * scale, sf::Vector2f(image.getSize()) * scale};
}

struct PaintFootprint
{
    sf::VertexArray faces{sf::PrimitiveType::Triangles};
    sf::VertexArray edges{sf::PrimitiveType::Lines};

    void addCell(sf::Vector2i cell, sf::Vector2f front, float side, const sf::IntRect& area)
    {
        constexpr sf::Color fill{255, 198, 98, 78};
        constexpr sf::Color border{255, 222, 156};
        const auto back = front - sf::Vector2f{0.f, side / 2.f};
        const auto left = front - sf::Vector2f{side / 2.f, side / 4.f};
        const auto right = front + sf::Vector2f{side / 2.f, -side / 4.f};
        for (const auto point : {back, right, front, back, front, left})
            faces.append({point, fill});

        const auto edge = [&](sf::Vector2f from, sf::Vector2f to) {
            edges.append({from, border});
            edges.append({to, border});
        };
        const auto end = area.position + area.size;
        if (cell.x == area.position.x) edge(left, back);
        if (cell.y == area.position.y) edge(back, right);
        if (cell.x + 1 == end.x) edge(right, front);
        if (cell.y + 1 == end.y) edge(front, left);
    }

    void drawAndClear(sf::RenderTarget& target)
    {
        if (faces.getVertexCount()) target.draw(faces);
        if (edges.getVertexCount()) target.draw(edges);
        faces.clear();
        edges.clear();
    }
};

// Walk the rectangle on the same diagonals as the sorted columns, skipping occupied cells.
std::vector<sf::Vector2i> emptyPaintCells(const sf::IntRect& area, const std::vector<const Tile*>& visible,
                                         const sf::FloatRect& bounds, float side)
{
    std::vector<sf::Vector2i> empty;
    auto occupied = visible.begin();
    for (int diagonal = 0; diagonal < area.size.x + area.size.y - 1; ++diagonal)
    {
        const int firstX = std::max(0, diagonal - area.size.y + 1);
        const int lastX = std::min(area.size.x - 1, diagonal);
        for (int x = firstX; x <= lastX; ++x)
        {
            const sf::Vector2i cell = area.position + sf::Vector2i{x, diagonal - x};
            while (occupied != visible.end() && behind((*occupied)->cell, cell)) ++occupied;
            if ((occupied == visible.end() || (*occupied)->cell != cell) && bounds.contains(baseFront(cell, side)))
                empty.push_back(cell);
        }
    }
    return empty;
}

void considerColumn(const Grid& grid, const Tile& tile, sf::Vector2f projectedPoint,
                    const IsometricArt& art, AssetLibrary* library, std::optional<sf::Vector2i>& picked)
{
    const float side = static_cast<float>(grid.cellSize);
    const float scale = side / 32.f;
    const float rise = side * terrainLayerHeightRatio;
    if (tile.cell.x < 0 || tile.cell.y < 0 || tile.cell.x >= grid.size || tile.cell.y >= grid.size)
        return;
    if (picked && !behind(*picked, tile.cell))
        return;
    const auto base = baseFront(tile.cell, side);
    const float elevation = columnElevation(tile, side);
    const auto top = base - sf::Vector2f{0.f, elevation};
    const auto& surface = tile.layers.back();
    const auto& decoration = surface.asset;
    if (library && !decoration.id.empty())
    {
        if (const auto* entry = library->find(decoration, false))
        {
            if (library->art.hitWorld(*entry, decoration, top, scale, projectedPoint).value_or(false))
            {
                picked = tile.cell;
                return;
            }
        }
    }
    else if (const sf::Image* image = art.objectImage(surface.object))
    {
        const auto bounds = objectBounds(*image, top, scale);
        const auto texel = (projectedPoint - bounds.position) / scale;
        if (sf::FloatRect({0.f, 0.f}, sf::Vector2f(image->getSize())).contains(texel))
        {
            if (image->getPixel(sf::Vector2u(texel)).a != 0)
            {
                picked = tile.cell;
                return;
            }
        }
    }
    const auto baseTexel = (projectedPoint - base) / scale + terrainFrontAnchor;
    if (baseTexel.x < 0.f || baseTexel.x >= terrainFrameSize.x ||
        baseTexel.y >= terrainFrameSize.y || baseTexel.y < -elevation / scale)
        return;

    // A transparent edge can expose another block. Sample only the frames under the pointer.
    const int x = static_cast<int>(baseTexel.x);
    for (std::size_t i = tile.layers.size(); i > 0; --i)
    {
        const auto& layer = tile.layers[i - 1];
        const float y = baseTexel.y + static_cast<float>(i - 1) * rise / scale;
        if (y < 0.f || y >= terrainFrameSize.y)
            continue;
        std::optional<bool> customHit;
        if (library && !layer.groundAsset.id.empty())
        {
            if (const auto* entry = library->find(layer.groundAsset, true))
            {
                const auto front = base - sf::Vector2f{0.f, static_cast<float>(i - 1) * rise};
                customHit = library->art.hitWorld(*entry, layer.groundAsset, front, scale, projectedPoint);
            }
        }
        if (customHit.value_or(art.groundPixelOpaque(layer.terrain, {x, static_cast<int>(y)})))
        {
            picked = tile.cell;
            break;
        }
    }
}

std::optional<sf::Vector2i> pickedOrBase(const Grid& grid, std::optional<sf::Vector2i> picked,
                                         sf::Vector2f projectedPoint)
{
    if (picked)
        return picked;
    const auto groundPoint = viewToMap(projectedPoint);
    if (grid.contains(groundPoint))
        return grid.worldToCell(groundPoint);
    return std::nullopt;
}
}

std::optional<sf::Vector2i> pickTerrainCell(const Grid& grid, const std::vector<Tile>& tiles,
                                            sf::Vector2f projectedPoint, const IsometricArt& art,
                                            AssetLibrary* library)
{
    if (grid.cellSize <= 0 || grid.size <= 0)
        return std::nullopt;
    std::optional<sf::Vector2i> picked;
    for (const Tile& tile : tiles)
        considerColumn(grid, tile, projectedPoint, art, library, picked);
    return pickedOrBase(grid, picked, projectedPoint);
}

TerrainStrokePicker::TerrainStrokePicker(const Grid& map, const std::vector<Tile>& cells, const IsometricArt& art,
                                         AssetLibrary* assets)
    : grid(map), tiles(cells), art(art), library(assets), regionSize(std::max(1.f, 2.f * map.cellSize))
{
    for (std::size_t i = 0; i < tiles.size(); ++i)
        includeTile(i);
}

std::pair<int, int> TerrainStrokePicker::regionAt(sf::Vector2f point) const
{
    return {static_cast<int>(std::floor(point.x / regionSize)), static_cast<int>(std::floor(point.y / regionSize))};
}

void TerrainStrokePicker::includeTile(std::size_t index)
{
    const Tile& tile = tiles[index];
    if (grid.cellSize <= 0 || tile.cell.x < 0 || tile.cell.y < 0 ||
        tile.cell.x >= grid.size || tile.cell.y >= grid.size)
        return;
    const float side = static_cast<float>(grid.cellSize);
    const float scale = side / 32.f;
    const sf::Vector2f base = baseFront(tile.cell, side);
    const sf::Vector2f top = base - sf::Vector2f{0.f, columnElevation(tile, side)};
    sf::Vector2f first = top - terrainFrontAnchor * scale;
    sf::Vector2f last = base + (sf::Vector2f(terrainFrameSize) - terrainFrontAnchor) * scale;
    const auto& surface = tile.layers.back();
    const auto& item = surface.asset;
    std::optional<sf::FloatRect> sprite;
    if (library && !item.id.empty())
    {
        if (const auto* entry = library->find(item, false))
            sprite = library->art.worldBounds(*entry, item.row, top, scale);
    }
    else if (const sf::Image* image = art.objectImage(surface.object))
        sprite = objectBounds(*image, top, scale);
    if (sprite)
    {
        first = {std::min(first.x, sprite->position.x), std::min(first.y, sprite->position.y)};
        last = {std::max(last.x, sprite->position.x + sprite->size.x),
                std::max(last.y, sprite->position.y + sprite->size.y)};
    }
    const auto [left, back] = regionAt(first);
    const auto [right, front] = regionAt(last);
    for (int y = back; y <= front; ++y)
        for (int x = left; x <= right; ++x)
            regions[{x, y}].push_back(index); // Indices survive vector growth as the stroke paints new cells.
}

std::optional<sf::Vector2i> TerrainStrokePicker::pick(sf::Vector2f point) const
{
    std::optional<sf::Vector2i> picked;
    const auto region = regions.find(regionAt(point));
    if (region != regions.end())
        for (const auto index : region->second)
            considerColumn(grid, tiles[index], point, art, library, picked);
    return pickedOrBase(grid, picked, point);
}

void drawIsometricMap(sf::RenderTarget& target, const Grid& grid, const std::vector<Tile>& tiles,
                      const IsometricArt& art, const Engineer* engineer, bool showGrid,
                      std::optional<sf::Vector2i> hoveredCell, AssetLibrary* library, const AssetPlacement* preview,
                      bool previewRaised, std::optional<sf::IntRect> paintArea)
{
    const float side = static_cast<float>(grid.cellSize);
    const float scale = side / 32.f;
    const float step = side * terrainLayerHeightRatio;
    const sf::View& view = target.getView();
    const sf::Vector2f padding{side * 2.f, side * 2.f + step * (MAX_TERRAIN_LAYERS - 1)};
    const sf::FloatRect bounds(view.getCenter() - view.getSize() / 2.f - padding,
                               view.getSize() + padding * 2.f);
    std::vector<const Tile*> visible;
    for (const Tile& tile : tiles)
    {
        const auto base = baseFront(tile.cell, side);
        bool onScreen = bounds.contains(base);
        if (!onScreen && library && !tile.layers.back().asset.id.empty())
        {
            const auto& item = tile.layers.back().asset;
            if (const auto* entry = library->find(item, false))
            {
                const auto top = base - sf::Vector2f{0.f, columnElevation(tile, side)};
                onScreen = bounds.findIntersection(library->art.worldBounds(*entry, item.row, top, scale)).has_value();
            }
        }
        if (onScreen)
            visible.push_back(&tile);
    }
    std::sort(visible.begin(), visible.end(), [](const Tile* a, const Tile* b) {
        return behind(a->cell, b->cell);
    });

    if (showGrid)
        grid.draw(target);
    if (!paintArea && hoveredCell && !findTile(tiles, *hoveredCell))
        grid.drawCellOutline(target, *hoveredCell, 0.f, sf::Color::Yellow);

    // Insert the engineer at his own cell, so columns in front can hide him.
    const auto feet = engineer ? engineer->getPosition() : sf::Vector2f{};
    const auto feetCell = grid.worldToCell(feet);
    const float feetHeight = engineer ? terrainElevationAt(grid, tiles, feet) : 0.f;
    bool engineerPending = engineer != nullptr;
    PaintFootprint footprint;
    const auto empty = paintArea ? emptyPaintCells(*paintArea, visible, bounds, side) : std::vector<sf::Vector2i>{};
    auto nextEmpty = empty.begin();
    const auto drawEmptyUntil = [&](const Tile* nextColumn) {
        while (nextEmpty != empty.end() && (!nextColumn || behind(*nextEmpty, nextColumn->cell)))
        {
            if (engineerPending && behind(feetCell, *nextEmpty))
            {
                footprint.drawAndClear(target);
                engineer->draw(target, feetHeight);
                engineerPending = false;
            }
            footprint.addCell(*nextEmpty, baseFront(*nextEmpty, side), side, *paintArea);
            ++nextEmpty;
        }
        footprint.drawAndClear(target);
    };
    for (const Tile* tile : visible)
    {
        if (nextEmpty != empty.end())
            drawEmptyUntil(tile);
        if (engineerPending && behind(feetCell, tile->cell))
        {
            engineer->draw(target, feetHeight);
            engineerPending = false;
        }
        const sf::Vector2f base = baseFront(tile->cell, side);
        sf::Vector2f front = base;
        for (std::size_t i = 0; i < tile->layers.size(); ++i)
        {
            const std::size_t layersAbove = tile->layers.size() - 1 - i;
            const auto& layer = tile->layers[i];
            bool drawn = false;
            if (library && !layer.groundAsset.id.empty())
            {
                if (const auto* entry = library->find(layer.groundAsset, true))
                {
                    const auto tint = layersAbove % 2 == 0 ? sf::Color::White : sf::Color{220, 220, 220};
                    drawn = library->art.drawWorld(target, *entry, layer.groundAsset, front, scale, tint);
                }
            }
            if (!drawn)
                drawIsometricGround(target, art, layer.terrain, front, scale, layersAbove);
            front.y -= step;
        }
        const float elevation = columnElevation(*tile, side);
        const sf::Vector2f top = base - sf::Vector2f{0.f, elevation};
        if (!paintArea && hoveredCell == tile->cell)
            grid.drawCellOutline(target, tile->cell, elevation, sf::Color::Yellow);
        else if (showGrid)
            grid.drawCellOutline(target, tile->cell, elevation, sf::Color::White);

        if (paintArea && paintArea->contains(tile->cell))
        {
            footprint.addCell(tile->cell, top, side, *paintArea);
            footprint.drawAndClear(target);
        }

        // Within the same cell, compare the engineer's feet to the decoration's ground contact.
        if (engineerPending && feetCell == tile->cell && mapToView(feet).y < base.y - side / 4.f)
        {
            engineer->draw(target, feetHeight);
            engineerPending = false;
        }
        const auto& surface = tile->layers.back();
        if (library && !surface.asset.id.empty())
        {
            const auto* entry = library->find(surface.asset, false);
            if (!entry || !library->art.drawWorld(target, *entry, surface.asset, top, scale))
            {
                // A missing optional pack keeps its saved placement and a visible location.
                sf::RectangleShape marker({6.f * scale, 6.f * scale});
                marker.setOrigin(marker.getSize() / 2.f);
                marker.setRotation(sf::degrees(45.f));
                marker.setPosition(top - sf::Vector2f{0.f, side / 4.f});
                marker.setFillColor(sf::Color{220, 143, 73});
                target.draw(marker);
            }
        }
        else
            drawIsometricObject(target, art, surface.object, top, scale);
    }
    if (nextEmpty != empty.end())
        drawEmptyUntil(nullptr);
    if (engineerPending)
        engineer->draw(target, feetHeight);
    if (!paintArea && library && preview && hoveredCell && !preview->id.empty())
    {
        const auto* entry = library->index().find(preview->id);
        if (entry && entry->placeable)
        {
            sf::Vector2f top = baseFront(*hoveredCell, side);
            if (const Tile* tile = findTile(tiles, *hoveredCell))
            {
                top.y -= columnElevation(*tile, side);
                if (previewRaised && tile->layers.size() < MAX_TERRAIN_LAYERS)
                    top.y -= step;
            }
            library->art.drawWorld(target, *entry, *preview, top, scale, sf::Color{255, 255, 255, 140});
        }
    }
}

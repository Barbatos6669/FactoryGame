#pragma once

#include "Tile.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <map>
#include <optional>

class Engineer;
class AssetLibrary;
class Grid;
struct IsometricArt;

// A projected point may land on the top or side of a column above a different map cell.
std::optional<sf::Vector2i> pickTerrainCell(const Grid& grid, const std::vector<Tile>& tiles,
                                            sf::Vector2f projectedPoint, const IsometricArt& art,
                                            AssetLibrary* library = nullptr);
float terrainElevationAt(const Grid& grid, const std::vector<Tile>& tiles, sf::Vector2f mapPosition);

// A drag samples many pixels. Keep only nearby columns in each projected region.
class TerrainStrokePicker
{
public:
    TerrainStrokePicker(const Grid& grid, const std::vector<Tile>& tiles, AssetLibrary* library);
    std::optional<sf::Vector2i> pick(sf::Vector2f point, const IsometricArt& art) const;
    void includeTile(std::size_t index);

private:
    const Grid& grid;
    const std::vector<Tile>& tiles;
    AssetLibrary* library;
    float regionSize;
    std::map<std::pair<int, int>, std::vector<std::size_t>> regions;
    std::pair<int, int> regionAt(sf::Vector2f point) const;
};

void drawIsometricMap(sf::RenderTarget& target, const Grid& grid, const std::vector<Tile>& tiles,
                      const IsometricArt& art, const Engineer* engineer, bool showGrid,
                      std::optional<sf::Vector2i> hoveredCell = std::nullopt,
                      AssetLibrary* library = nullptr, const AssetPlacement* preview = nullptr,
                      bool previewRaised = false);

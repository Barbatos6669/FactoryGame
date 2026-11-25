#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <utility>
#include <vector>

enum class TerrainType { Grass, Dirt, Water };
enum class TileObject { None, Rock, Flower };

struct AssetPlacement
{
    std::string id;
    int row = 0;
    int frame = 0;

    bool operator==(const AssetPlacement& other) const
    {
        return id == other.id && row == other.row && frame == other.frame;
    }
};

struct TerrainLayer
{
    TerrainType terrain = TerrainType::Grass;
    TileObject object = TileObject::None;
    AssetPlacement asset{};
    AssetPlacement groundAsset{};

    bool operator==(const TerrainLayer& other) const
    {
        return terrain == other.terrain && object == other.object &&
               asset == other.asset && groundAsset == other.groundAsset;
    }
};

inline constexpr int MAX_TERRAIN_LAYERS = 8;

/// A painted cell has one or more blocks, ordered from bottom to top.
struct Tile
{
    sf::Vector2i cell;
    std::vector<TerrainLayer> layers = {TerrainLayer{}};
};

/// A map selection stores complete columns at offsets from its minimum x and y.
/// Cells missing from tiles are transparent when the patch is pasted.
struct TilePatch
{
    sf::Vector2i size{};
    std::vector<Tile> tiles;

    bool fits(sf::Vector2i anchor, int gridSize) const;
};

enum class Brush { Grass, Dirt, Water, Rock, Flower };
enum class TerrainTool { Paint, Stack, Lower, Erase };

std::vector<Tile> makeSampleTiles(int gridSize);
const Tile* findTile(const std::vector<Tile>& tiles, sf::Vector2i cell);
void paintTile(std::vector<Tile>& tiles, sf::Vector2i cell, Brush brush);
/// Apply a terrain brush or a ground stamp to one surface, preserving its decoration.
void paintGround(TerrainLayer& surface, Brush brush, const AssetPlacement& groundStamp);
/// Indices to repaint in one connected, same-height patch. An unchanged or empty seed yields no indices.
std::vector<std::size_t> terrainFillCells(const std::vector<Tile>& tiles, sf::Vector2i seed,
                                          Brush brush, const AssetPlacement& groundStamp);
/// Paint the exposed surfaces. A nonempty ground stamp supplies the artwork in place of a terrain brush.
bool paintTerrainRectangle(std::vector<Tile>& tiles, sf::IntRect bounds, Brush brush, AssetPlacement groundStamp);
/// Add one block per cell, including empty cells. Return the number below the height limit that were raised.
std::size_t raiseTerrainRectangle(std::vector<Tile>& tiles, sf::IntRect bounds, TerrainType terrain,
                                  const AssetPlacement& groundStamp);
/// Remove one upper block per painted cell in the rectangle. Bases stay in place.
std::size_t lowerTerrainRectangle(std::vector<Tile>& tiles, sf::IntRect bounds);
bool placeAsset(std::vector<Tile>& tiles, sf::Vector2i cell, const AssetPlacement& asset, bool groundSurface = false);
void clearTileObject(std::vector<Tile>& tiles, sf::Vector2i cell);

/// Add a block, or create the base if the cell is empty. False when full.
bool stackTile(std::vector<Tile>& tiles, sf::Vector2i cell, TerrainType terrain);
/// Remove the top block. False if the cell is empty or has only its base.
bool lowerTile(std::vector<Tile>& tiles, sf::Vector2i cell);
/// Remove the exposed block, including the base if it is the last one. False if empty.
bool eraseTile(std::vector<Tile>& tiles, sf::Vector2i cell);
/// Erase one layer per cell and close the gaps left by removed base blocks together.
void eraseTiles(std::vector<Tile>& tiles, const std::set<std::pair<int, int>>& cells);

bool saveTiles(const std::vector<Tile>& tiles, const std::filesystem::path& path);
bool loadTiles(std::vector<Tile>& tiles, const std::filesystem::path& path);

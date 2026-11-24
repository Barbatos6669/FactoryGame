#include "Tile.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
Tile* findMutableTile(std::vector<Tile>& tiles, sf::Vector2i cell)
{
    for (Tile& tile : tiles)
    {
        if (tile.cell == cell)
            return &tile;
    }
    return nullptr;
}

Tile& getOrCreateTile(std::vector<Tile>& tiles, sf::Vector2i cell)
{
    if (Tile* tile = findMutableTile(tiles, cell))
        return *tile;

    tiles.push_back({cell});
    return tiles.back();
}

// These words are part of the save format. Each one names a specific enum value.
const std::map<std::string_view, TerrainType> terrainByName = {
    {"grass", TerrainType::Grass},
    {"dirt",  TerrainType::Dirt},
    {"water", TerrainType::Water}
};

const std::map<std::string_view, TileObject> objectByName = {
    {"none",   TileObject::None},
    {"rock",   TileObject::Rock},
    {"flower", TileObject::Flower}
};

template <typename Value>
std::string_view savedName(const std::map<std::string_view, Value>& valuesByName, Value value)
{
    for (const auto& [name, storedValue] : valuesByName)
    {
        if (storedValue == value)
            return name;
    }
    return {};
}

bool readLayer(std::istream& row, const std::string& ground, TerrainLayer& layer)
{
    std::string object;
    if (!(row >> object))
        return false;

    const auto terrainEntry = terrainByName.find(ground);
    const auto objectEntry = objectByName.find(object);
    if (terrainEntry == terrainByName.end() || objectEntry == objectByName.end())
        return false;
    layer.terrain = terrainEntry->second;
    layer.object = objectEntry->second;
    return true;
}

// Save files carry an asset key, never a path to open. Keep the spelling portable.
bool validPlacement(const AssetPlacement& placement)
{
    if (placement.id.empty())
        return placement.row == 0 && placement.frame == 0;
    if (placement.id.size() > 1024 || placement.row < 0 || placement.frame < 0)
        return false;

    const std::string_view key = placement.id;
    const auto hash = key.find('#');
    std::string_view path = key.substr(0, hash);
    const auto lettersAndDigits = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    };
    if (hash != std::string_view::npos)
    {
        const auto sprite = key.substr(hash + 1);
        if (sprite.empty() || !std::all_of(sprite.begin(), sprite.end(), [&](char c) {
            return lettersAndDigits(c) || c == '_' || c == '-';
        }))
            return false;
    }

    if (path.find('/') == std::string_view::npos)
        return false;
    while (true)
    {
        const auto slash = path.find('/');
        const auto part = path.substr(0, slash);
        if (part.empty() || part.front() == ' ' || part.back() == ' ' || part.back() == '.' ||
            !std::all_of(part.begin(), part.end(), [&](char c) {
                return lettersAndDigits(c) || c == ' ' || c == '.' || c == '_' || c == '-';
            }))
            return false;
        if (slash == std::string_view::npos)
            return true;
        path.remove_prefix(slash + 1);
    }
}

bool readPlacementIndex(std::istream& row, int& index)
{
    const int separator = row.peek();
    return (separator == ' ' || separator == '\t') && static_cast<bool>(row >> index);
}

bool readPlacement(std::istream& row, AssetPlacement& placement)
{
    row >> std::ws;
    if (row.get() != '"' || !std::getline(row, placement.id, '"') ||
        !readPlacementIndex(row, placement.row) || !readPlacementIndex(row, placement.frame))
        return false;
    const int next = row.peek();
    return (next == std::char_traits<char>::eof() || next == ' ' || next == '\t' || next == '\r') &&
           validPlacement(placement);
}

bool readTile(const std::string& line, Tile& tile)
{
    std::istringstream row(line);
    char comma;
    if (!(row >> tile.cell.x >> comma >> tile.cell.y) || comma != ',')
        return false;

    // An old grass save ends after x,y. The Tile defaults already describe it.
    const int next = row.peek();
    if (next != std::char_traits<char>::eof() && next != ' ' && next != '\t' && next != '\r')
        return false;
    std::string ground;
    if (!(row >> ground))
        return true;

    const bool withAssets = ground == "assets";
    if (!withAssets && ground != "stack")
    {
        std::string extra;
        return readLayer(row, ground, tile.layers.back()) && !(row >> extra);
    }

    // Asset rows add independent ground and object placements to every layer.
    tile.layers.clear();
    while (row >> ground)
    {
        TerrainLayer layer;
        if (tile.layers.size() >= MAX_TERRAIN_LAYERS || !readLayer(row, ground, layer))
            return false;
        if (withAssets && (!readPlacement(row, layer.groundAsset) || !readPlacement(row, layer.asset) ||
                          (!layer.asset.id.empty() && layer.object != TileObject::None)))
            return false;
        tile.layers.push_back(layer);
    }
    return tile.layers.size() >= (withAssets ? 1u : 2u);
}

bool writeTile(std::ostream& file, const Tile& tile)
{
    if (tile.layers.empty() || tile.layers.size() > MAX_TERRAIN_LAYERS)
        return false;

    const bool withAssets = std::any_of(tile.layers.begin(), tile.layers.end(), [](const TerrainLayer& layer) {
        return !layer.groundAsset.id.empty() || !layer.asset.id.empty();
    });
    file << tile.cell.x << ',' << tile.cell.y;
    if (withAssets)
        file << " assets";
    else if (tile.layers.size() > 1)
        file << " stack";
    for (const TerrainLayer& layer : tile.layers)
    {
        const std::string_view ground = savedName(terrainByName, layer.terrain);
        const std::string_view object = savedName(objectByName, layer.object);
        if (ground.empty() || object.empty() || !validPlacement(layer.groundAsset) || !validPlacement(layer.asset) ||
            (!layer.asset.id.empty() && layer.object != TileObject::None))
            return false;
        file << ' ' << ground << ' ' << object;
        if (withAssets)
        {
            const auto& surface = layer.groundAsset;
            const auto& decoration = layer.asset;
            file << ' ' << std::quoted(surface.id) << ' ' << surface.row << ' ' << surface.frame
                 << ' ' << std::quoted(decoration.id) << ' ' << decoration.row << ' ' << decoration.frame;
        }
    }
    return static_cast<bool>(file << '\n');
}
}

std::vector<Tile> makeSampleTiles(int gridSize)
{
    const int m = gridSize / 2;
    const Tile examples[] = {
        {{m - 1, m - 1}, {{TerrainType::Grass, TileObject::Flower}}},
        {{m,     m - 1}, {{TerrainType::Dirt,  TileObject::Flower}}},
        {{m + 1, m - 1}, {{TerrainType::Water, TileObject::None}}},
        {{m - 1, m},     {{TerrainType::Dirt,  TileObject::None}}},
        {{m,     m},     {{TerrainType::Grass, TileObject::None}}},
        {{m + 1, m},     {{TerrainType::Dirt,  TileObject::None},
                          {TerrainType::Grass, TileObject::None},
                          {TerrainType::Dirt,  TileObject::Rock}}}
    };

    std::vector<Tile> tiles;
    for (const Tile& tile : examples)
    {
        if (tile.cell.x >= 0 && tile.cell.x < gridSize &&
            tile.cell.y >= 0 && tile.cell.y < gridSize)
            tiles.push_back(tile);
    }
    return tiles;
}

const Tile* findTile(const std::vector<Tile>& tiles, sf::Vector2i cell)
{
    for (const Tile& tile : tiles)
    {
        if (tile.cell == cell)
            return &tile;
    }
    return nullptr;
}

void paintTile(std::vector<Tile>& tiles, sf::Vector2i cell, Brush brush)
{
    Tile& tile = getOrCreateTile(tiles, cell);
    TerrainLayer& surface = tile.layers.back();
    switch (brush)
    {
        case Brush::Grass:
            surface.terrain = TerrainType::Grass;
            surface.groundAsset = {};
            break;
        case Brush::Dirt:
            surface.terrain = TerrainType::Dirt;
            surface.groundAsset = {};
            break;
        case Brush::Water:
            surface.terrain = TerrainType::Water;
            surface.groundAsset = {};
            break;
        case Brush::Rock:
            surface.object = TileObject::Rock;
            surface.asset = {};
            break;
        case Brush::Flower:
            surface.object = TileObject::Flower;
            surface.asset = {};
            break;
    }
}

bool placeAsset(std::vector<Tile>& tiles, sf::Vector2i cell, const AssetPlacement& asset, bool groundSurface)
{
    if (asset.id.empty() || !validPlacement(asset))
        return false;

    AssetPlacement placed = asset;
    TerrainLayer& surface = getOrCreateTile(tiles, cell).layers.back();
    if (groundSurface)
        surface.groundAsset = std::move(placed);
    else
    {
        surface.asset = std::move(placed);
        surface.object = TileObject::None;
    }
    return true;
}

void clearTileObject(std::vector<Tile>& tiles, sf::Vector2i cell)
{
    if (Tile* tile = findMutableTile(tiles, cell))
    {
        tile->layers.back().object = TileObject::None;
        tile->layers.back().asset = {};
    }
}

bool stackTile(std::vector<Tile>& tiles, sf::Vector2i cell, TerrainType terrain)
{
    const TerrainLayer block{terrain, TileObject::None};
    Tile* tile = findMutableTile(tiles, cell);
    if (tile == nullptr)
    {
        tiles.push_back({cell, {block}});
        return true;
    }
    if (tile->layers.size() >= MAX_TERRAIN_LAYERS)
        return false;

    tile->layers.push_back(block);
    return true;
}

bool lowerTile(std::vector<Tile>& tiles, sf::Vector2i cell)
{
    Tile* tile = findMutableTile(tiles, cell);
    if (tile == nullptr || tile->layers.size() <= 1)
        return false;

    tile->layers.pop_back();
    return true;
}

bool eraseTile(std::vector<Tile>& tiles, sf::Vector2i cell)
{
    const auto tile = std::find_if(tiles.begin(), tiles.end(), [cell](const Tile& candidate) {
        return candidate.cell == cell;
    });
    if (tile == tiles.end())
        return false;

    if (tile->layers.size() > 1)
        tile->layers.pop_back();
    else
        tiles.erase(tile);
    return true;
}

void eraseTiles(std::vector<Tile>& tiles, const std::set<std::pair<int, int>>& cells)
{
    if (cells.empty())
        return;
    if (cells.size() == 1)
    {
        const auto [x, y] = *cells.begin();
        eraseTile(tiles, {x, y});
        return;
    }

    for (Tile& tile : tiles)
    {
        if (cells.count({tile.cell.x, tile.cell.y}) && !tile.layers.empty())
            tile.layers.pop_back();
    }
    tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [](const Tile& tile) {
        return tile.layers.empty();
    }), tiles.end());
}

bool saveTiles(const std::vector<Tile>& tiles, const std::filesystem::path& path)
{
    // Finish writing before replacing the previous save.
    auto tempPath = path;
    const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
    tempPath += "." + std::to_string(timestamp) + ".tmp";
    std::ofstream file(tempPath);
    if (!file)
        return false;
    for (const Tile& tile : tiles)
    {
        if (!writeTile(file, tile))
        {
            // Discard the temporary file if any row could not be written.
            file.setstate(std::ios::failbit);
            break;
        }
    }
    file.close();
    std::error_code error;
    if (file)
    {
        std::filesystem::rename(tempPath, path, error);
        if (!error)
            return true;
    }
    std::filesystem::remove(tempPath, error);
    return false;
}

bool loadTiles(std::vector<Tile>& tiles, const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
        return false;

    std::vector<Tile> loaded;
    std::string line;
    while (std::getline(file, line))
    {
        Tile tile;
        if (!readTile(line, tile))
            return false;
        getOrCreateTile(loaded, tile.cell) = std::move(tile);
    }
    if (file.bad())
        return false;
    tiles.swap(loaded);
    return true;
}

#include "Tile.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
Tile* findTile(std::vector<Tile>& tiles, sf::Vector2i cell)
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
    if (Tile* tile = findTile(tiles, cell))
        return *tile;

    tiles.push_back({cell});
    return tiles.back();
}

// Each name has the same index as its enum value in Tile.hpp.
const std::vector<std::string> terrainNames = {"grass", "dirt", "water"};
const std::vector<std::string> objectNames = {"none", "rock", "flower"};

int findName(const std::vector<std::string>& names, const std::string& word)
{
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (names[i] == word)
            return static_cast<int>(i);
    }
    return -1;
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
    std::string ground, object, extra;
    if (!(row >> ground))
        return true;
    if (!(row >> object) || row >> extra)
        return false;

    const int terrainIndex = findName(terrainNames, ground);
    const int objectIndex = findName(objectNames, object);
    if (terrainIndex < 0 || objectIndex < 0)
        return false;
    tile.terrain = static_cast<TerrainType>(terrainIndex);
    tile.object = static_cast<TileObject>(objectIndex);
    return true;
}
}

std::vector<Tile> makeSampleTiles(int gridSize)
{
    const int m = gridSize / 2;
    const Tile examples[] = {
        {{m - 1, m - 1}, TerrainType::Grass, TileObject::Flower},
        {{m,     m - 1}, TerrainType::Dirt,  TileObject::Flower},
        {{m + 1, m - 1}, TerrainType::Water, TileObject::None},
        {{m - 1, m},     TerrainType::Dirt,  TileObject::None},
        {{m,     m},     TerrainType::Grass, TileObject::None},
        {{m + 1, m},     TerrainType::Dirt,  TileObject::Rock}
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

void paintTile(std::vector<Tile>& tiles, sf::Vector2i cell, Brush brush)
{
    Tile& tile = getOrCreateTile(tiles, cell);
    switch (brush)
    {
        case Brush::Grass:  tile.terrain = TerrainType::Grass; break;
        case Brush::Dirt:   tile.terrain = TerrainType::Dirt;  break;
        case Brush::Water:  tile.terrain = TerrainType::Water; break;
        case Brush::Rock:   tile.object = TileObject::Rock;    break;
        case Brush::Flower: tile.object = TileObject::Flower;  break;
    }
}

void clearTileObject(std::vector<Tile>& tiles, sf::Vector2i cell)
{
    if (Tile* tile = findTile(tiles, cell))
        tile->object = TileObject::None;
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
        file << tile.cell.x << ',' << tile.cell.y << ' '
             << terrainNames[static_cast<int>(tile.terrain)] << ' '
             << objectNames[static_cast<int>(tile.object)] << '\n';
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
        getOrCreateTile(loaded, tile.cell) = tile;
    }
    if (file.bad())
        return false;
    tiles.swap(loaded);
    return true;
}

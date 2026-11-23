#pragma once

#include <SFML/System/Vector2.hpp>
#include <filesystem>
#include <vector>

enum class TerrainType { Grass, Dirt, Water };
enum class TileObject { None, Rock, Flower };

/// One painted cell has a ground surface and can hold one object on top.
struct Tile
{
    sf::Vector2i cell;
    TerrainType terrain = TerrainType::Grass;
    TileObject object = TileObject::None;
};

enum class Brush { Grass, Dirt, Water, Rock, Flower };

std::vector<Tile> makeSampleTiles(int gridSize);
void paintTile(std::vector<Tile>& tiles, sf::Vector2i cell, Brush brush);
void clearTileObject(std::vector<Tile>& tiles, sf::Vector2i cell);

bool saveTiles(const std::vector<Tile>& tiles, const std::filesystem::path& path);
bool loadTiles(std::vector<Tile>& tiles, const std::filesystem::path& path);

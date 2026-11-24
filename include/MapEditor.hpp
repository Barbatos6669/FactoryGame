#pragma once

#include "EditHistory.hpp"
#include "EditorUI.hpp"
#include "TerrainScene.hpp"
#include <filesystem>
#include <optional>
#include <set>

class Debug;

/// Owns the current map and groups each click or drag into one undoable edit.
class MapEditor
{
public:
    MapEditor(std::vector<Tile> initialTiles, std::filesystem::path savePath);
    // The active picker refers to this map or to its stroke's snapshot.
    MapEditor(const MapEditor&) = delete;
    MapEditor& operator=(const MapEditor&) = delete;

    const std::vector<Tile>& getTiles() const { return tiles; }
    std::filesystem::path savePath;
    static constexpr float minimumZoom = 1.f;
    float zoom = minimumZoom;

    Brush getBrush() const { return brush; }
    TerrainTool getTool() const { return tool; }
    const AssetPlacement& getSelectedAsset() const { return selectedAsset; }
    void selectBrush(Brush choice);
    void selectTool(TerrainTool choice, const AssetLibrary& library);
    void selectStamp(AssetPlacement stamp);
    void cancelSelection();

    void pressCell(sf::Vector2i cell, sf::Vector2i pixel, const sf::View& view, EditorUI& ui, AssetLibrary& library);
    void clearDecoration(sf::Vector2i cell);
    void dragTo(sf::Vector2i end, const sf::RenderTarget& target, bool focused, const Grid& grid,
                const Debug& debug, EditorUI& ui, const IsometricArt& art, AssetLibrary& library);
    void finishEdit();
    void finishIfCameraMoved(const sf::View& view);
    bool isDragging() const { return stroke.has_value(); }
    bool undoEdit();
    bool loadMap();

private:
    struct TerrainStroke
    {
        TerrainStroke(sf::Vector2i pixel, sf::Vector2i cell, const sf::View& view);
        void preparePicker(const Grid& grid, const std::vector<Tile>& tiles, AssetLibrary& library, bool erasing);
        sf::Vector2i pixel;
        sf::Vector2i lastCell;
        sf::Vector2f viewCenter;
        sf::Vector2f viewSize;
        std::set<std::pair<int, int>> visited;
        std::optional<Tile> firstErased;
        std::optional<std::vector<Tile>> eraseSnapshot;
        std::optional<TerrainStrokePicker> picker;
    };

    bool paintsSurface(const AssetLibrary& library) const;
    void editCell(sf::Vector2i cell, EditorUI& ui, AssetLibrary& library);
    Brush brush = Brush::Grass;
    Brush lastGround = Brush::Grass;
    TerrainTool tool = TerrainTool::Paint;
    AssetPlacement selectedAsset{};
    std::vector<Tile> tiles;
    std::optional<TerrainStroke> stroke;
    EditHistory history;
};

// A press, drag, hover and wheel share the same screen-space bounds and projection.
EditorCommand editorHit(const EditorUI& ui, const Debug& debug, sf::Vector2u size, sf::Vector2i pixel);
sf::Vector2f projectedMousePoint(const sf::RenderTarget& target, sf::Vector2i pixel);
bool insideWindow(sf::Vector2i pixel, sf::Vector2u size);

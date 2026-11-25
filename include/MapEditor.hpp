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
    /// Pick a Paint brush from the exposed layer. groundOnly skips its decoration.
    void sampleCell(sf::Vector2i cell, bool groundOnly, EditorUI& ui, const AssetLibrary& library);
    bool paintsSurface(const AssetLibrary& library) const;
    /// Repaint the connected patch under the cursor and return the number of changed cells.
    std::size_t fillSurface(sf::Vector2i cell, const AssetLibrary& library);

    const TilePatch& getClipboard() const { return clipboard; }
    /// Keep the previous clipboard when the selection contains no painted columns.
    bool copyArea(sf::IntRect bounds);
    /// Return the changed column count, or no value if the full patch cannot fit.
    std::optional<std::size_t> pasteAt(sf::Vector2i anchor, int gridSize);
    void beginCopyRectangle(sf::Vector2i cell, const sf::View& view);
    bool isCopyingRectangle() const { return rectangle && rectangle->copying; }
    bool beginRectangle(sf::Vector2i cell, const sf::View& view, const AssetLibrary& library);
    void updateRectangle(sf::Vector2i cell);
    std::optional<sf::IntRect> rectangleBounds() const;
    void commitRectangle(EditorUI& ui, AssetLibrary& library);
    void pressCell(sf::Vector2i cell, sf::Vector2i pixel, const sf::View& view, EditorUI& ui, AssetLibrary& library);
    void clearDecoration(sf::Vector2i cell);
    void dragTo(sf::Vector2i end, const sf::RenderTarget& target, bool focused, const Grid& grid,
                const Debug& debug, EditorUI& ui, const IsometricArt& art, AssetLibrary& library);
    void finishEdit();
    void finishIfCameraMoved(const sf::View& view);
    bool isDragging() const { return stroke.has_value(); }
    bool canUndo() const { return history.canUndo(); }
    bool canRedo() const { return history.canRedo(); }
    bool undoEdit();
    bool redoEdit();
    bool loadMap();

private:
    struct RectanglePreview
    {
        sf::Vector2i anchor;
        sf::Vector2i end;
        sf::Vector2f viewCenter;
        sf::Vector2f viewSize;
        bool copying = false;
    };

    struct TerrainStroke
    {
        TerrainStroke(sf::Vector2i pixel, sf::Vector2i cell, const sf::View& view);
        void preparePicker(const Grid& grid, const std::vector<Tile>& tiles, const IsometricArt& art,
                           AssetLibrary& library, bool erasing);
        sf::Vector2i pixel;
        sf::Vector2i lastCell;
        sf::Vector2f viewCenter;
        sf::Vector2f viewSize;
        std::set<std::pair<int, int>> visited;
        std::optional<Tile> firstErased;
        std::optional<std::vector<Tile>> eraseSnapshot;
        std::optional<TerrainStrokePicker> picker;
    };

    void editCell(sf::Vector2i cell, EditorUI& ui, AssetLibrary& library);
    bool canEditRectangle(const AssetLibrary& library) const;
    Brush brush = Brush::Grass;
    Brush lastGround = Brush::Grass;
    TerrainTool tool = TerrainTool::Paint;
    AssetPlacement selectedAsset{};
    std::vector<Tile> tiles;
    TilePatch clipboard;
    std::optional<TerrainStroke> stroke;
    std::optional<RectanglePreview> rectangle;
    EditHistory history;
};

// A press, drag, hover and wheel share the same screen-space bounds and projection.
EditorCommand editorHit(const EditorUI& ui, const Debug& debug, sf::Vector2u size, sf::Vector2i pixel);
sf::Vector2f projectedMousePoint(const sf::RenderTarget& target, sf::Vector2i pixel);
bool insideWindow(sf::Vector2i pixel, sf::Vector2u size);

/**
 * @file main.cpp
 * @brief Main entry point for the game application.
 *
 * This file contains the main function that initializes the game,
 * sets up resources, and enters the main game loop. It uses the SFML
 * library for graphics rendering and window management.
 *
 * @author Barbatos6669
 * @version 0.1.0
 * @copyright (c) 2026 Barbatos6669. All rights reserved.
 */

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "AssetArt.hpp"
#include "AssetBrowser.hpp"
#include "Debug.hpp"
#include "EditorUI.hpp"
#include "Engineer.hpp"
#include "Grid.hpp"
#include "IsometricArt.hpp"
#include "MapEditor.hpp"
#include "Projection.hpp"
#include "TerrainScene.hpp"
#include "Tile.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

constexpr unsigned int MAX_START_WIDTH = 1920;
constexpr unsigned int MAX_START_HEIGHT = 1080;
constexpr float CAMERA_SPEED = 400.f;       ///< Camera movement in world units per second.
constexpr float MAX_ZOOM = 1.5f;
constexpr float ZOOM_PER_NOTCH = 0.125f;

#ifdef _WIN32
// Set the bounds Windows will use when the player clicks Restore.
static void centerRestoredWindow(
    HWND window
)
{
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    RECT frame{};
    if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor) ||
        !GetWindowRect(window, &frame))
        return;

    const RECT& work = monitor.rcWork;
    const LONG workWidth = work.right - work.left;
    const LONG workHeight = work.bottom - work.top;
    const LONG width = std::min(frame.right - frame.left, workWidth);
    const LONG height = std::min(frame.bottom - frame.top, workHeight);
    SetWindowPos(window, nullptr, work.left + (workWidth - width) / 2, work.top + (workHeight - height) / 2,
                 width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}
#endif

/** @brief Open a resizable window and start it maximized on Windows. */
void setupWindow(
    sf::RenderWindow& window
)
{
    const auto desktop = sf::VideoMode::getDesktopMode().size;
    const sf::Vector2u size{
        std::min(MAX_START_WIDTH, desktop.x - desktop.x / 10),
        std::min(MAX_START_HEIGHT, desktop.y - desktop.y / 10)
    };
    window.create(sf::VideoMode(size), "Beltbound");
#ifdef _WIN32
    const HWND handle = window.getNativeHandle();
    centerRestoredWindow(handle);
    ShowWindow(handle, SW_MAXIMIZE);
#endif
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
}

struct EditorLibrary
{
    AssetLibrary assets;
    AssetBrowser browser;
};

// Keyboard shortcuts and clickable controls both come through here.
void applyEditorCommand(
    EditorCommand command,
    MapEditor& editor,
    EditorUI& ui,
    Debug& debug,
    EditorLibrary& library
)
{
    switch (command)
    {
        case EditorCommand::Grass:
            editor.selectBrush(Brush::Grass);
            break;
        case EditorCommand::Dirt:
            editor.selectBrush(Brush::Dirt);
            break;
        case EditorCommand::Water:
            editor.selectBrush(Brush::Water);
            break;
        case EditorCommand::Rock:
            editor.selectBrush(Brush::Rock);
            break;
        case EditorCommand::Flower:
            editor.selectBrush(Brush::Flower);
            break;
        case EditorCommand::Paint:
            editor.selectTool(TerrainTool::Paint, library.assets);
            break;
        case EditorCommand::Stack:
            editor.selectTool(TerrainTool::Stack, library.assets);
            break;
        case EditorCommand::Lower:
            editor.selectTool(TerrainTool::Lower, library.assets);
            break;
        case EditorCommand::Erase:
            editor.selectTool(TerrainTool::Erase, library.assets);
            break;
        case EditorCommand::Undo:
            ui.notify(editor.undoEdit() ? "Undid last edit" : "Nothing to undo");
            break;
        case EditorCommand::Redo:
            ui.notify(editor.redoEdit() ? "Redid last edit" : "Nothing to redo");
            break;
        case EditorCommand::ToggleLibrary:
            editor.finishEdit();
            if (debug.getFont())
            {
                library.assets.requestIndex();
                library.browser.toggle();
            }
            else
                debug.log("The asset library needs assets/fonts/Tuffy.ttf.");
            break;
        case EditorCommand::Save:
        {
            editor.finishEdit();
            const bool saved = saveTiles(editor.getTiles(), editor.savePath);
            ui.notify(saved ? "Map saved" : "Could not write map file");
            debug.log((saved ? "Saved tiles to " : "Could not save tiles to ") + editor.savePath.string());
            break;
        }
        case EditorCommand::Load:
        {
            const bool loaded = editor.loadMap();
            if (loaded)
            {
                ui.notify("Map loaded");
                // Terrain-only maps can be loaded without opening or indexing the asset library.
                const bool usesLibrary = std::any_of(editor.getTiles().begin(), editor.getTiles().end(), [](const Tile& tile) {
                    return std::any_of(tile.layers.begin(), tile.layers.end(), [](const TerrainLayer& layer) {
                        return !layer.asset.id.empty() || !layer.groundAsset.id.empty();
                    });
                });
                if (usesLibrary)
                    library.assets.requestIndex();
            }
            else
            {
                std::error_code error;
                const bool exists = std::filesystem::exists(editor.savePath, error);
                ui.notify(!error && !exists ? "No saved map yet" : "Load failed. Current map kept");
            }
            debug.log((loaded ? "Loaded tiles from " : "Could not load tiles from ") + editor.savePath.string());
            break;
        }
        case EditorCommand::ToggleGrid:
            editor.finishEdit();
            debug.toggleOverlay();
            debug.toggleGridVisibility();
            break;
        case EditorCommand::Canvas:
        case EditorCommand::Panel:
            break;
    }
}

void resizeWorldView(sf::RenderWindow& window, sf::View& view, sf::Vector2u pixelSize, float zoom)
{
    if (pixelSize.x == 0 || pixelSize.y == 0)
        return;
    view.setSize(sf::Vector2f(pixelSize) / zoom);
    window.setView(view);
}

bool shiftHeld()
{
    using Key = sf::Keyboard::Key;
    return sf::Keyboard::isKeyPressed(Key::LShift) || sf::Keyboard::isKeyPressed(Key::RShift);
}

/** @brief Handle editor input and report focus changes for the camera. */
bool handleEvents(
    sf::RenderWindow& window,
    sf::View& view,
    Debug& debug,
    const Grid& grid,
    MapEditor& editor,
    EditorUI& ui,
    const IsometricArt& art,
    EditorLibrary& library
)
{
    bool focusChanged = false;
    const auto cellAtPixel = [&](sf::Vector2i pixel) {
        return pickTerrainCell(grid, editor.getTiles(), projectedMousePoint(window, pixel), art, &library.assets);
    };
    const auto canvasCellAt = [&](sf::Vector2i pixel) -> std::optional<sf::Vector2i> {
        if (!window.hasFocus() || !insideWindow(pixel, window.getSize()) ||
            editorHit(ui, debug, window.getSize(), pixel) != EditorCommand::Canvas)
            return std::nullopt;
        return cellAtPixel(pixel);
    };
    const auto sampleAtPixel = [&](sf::Vector2i pixel, bool groundOnly) {
        if (const auto cell = canvasCellAt(pixel))
            editor.sampleCell(*cell, groundOnly, ui, library.assets);
    };
    const auto updateDrag = [&](sf::Vector2i pixel) {
        if (editor.rectangleBounds())
        {
            if (const auto cell = canvasCellAt(pixel))
                editor.updateRectangle(*cell);
            else
                editor.finishEdit();
        }
        else
            editor.dragTo(pixel, window, window.hasFocus(), grid, debug, ui, art, library.assets);
    };

    while (const auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
            break;
        }

        if (event->is<sf::Event::FocusLost>() || event->is<sf::Event::FocusGained>())
        {
            editor.finishEdit();
            focusChanged = true;
        }
        if (event->is<sf::Event::MouseLeft>())
            editor.finishEdit();

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            editor.finishEdit();
            resizeWorldView(window, view, resized->size, editor.zoom);
        }
        if (library.browser.isOpen())
        {
            editor.finishEdit();
            const auto stamp = library.browser.handleEvent(*event, window.getSize(), library.assets.index(),
                                                           editor.getSelectedAsset());
            if (stamp)
                editor.selectStamp(*stamp);
            focusChanged = true; // Typing and navigating the library never moves the engineer.
            continue;
        }
        if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (!window.hasFocus())
                continue;
            if (key->code == sf::Keyboard::Key::Escape)
                editor.cancelSelection();
            else if (key->code == sf::Keyboard::Key::I && !key->control && !key->alt && !key->system)
                sampleAtPixel(sf::Mouse::getPosition(window), key->shift);
            else if (key->code == sf::Keyboard::Key::F && !key->shift && !key->control && !key->alt && !key->system)
            {
                const auto cell = canvasCellAt(sf::Mouse::getPosition(window));
                if (!cell || editor.isDragging() || editor.rectangleBounds())
                    continue;
                if (!editor.paintsSurface(library.assets))
                    ui.notify("Select a terrain brush in Paint to fill");
                else if (const auto count = editor.fillSurface(*cell, library.assets))
                    ui.notify("Filled " + std::to_string(count) + (count == 1 ? " cell" : " cells"));
                else if (findTile(editor.getTiles(), *cell))
                    ui.notify("This ground already matches your brush");
                else
                    ui.notify("Point at painted ground to fill");
            }
            else
                applyEditorCommand(editorCommandForKey(*key), editor, ui, debug, library);
        }
        if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (wheel->wheel == sf::Mouse::Wheel::Vertical && std::isfinite(wheel->delta) &&
                editorHit(ui, debug, window.getSize(), wheel->position) == EditorCommand::Canvas)
            {
                const float zoom = std::clamp(editor.zoom + wheel->delta * ZOOM_PER_NOTCH,
                                              MapEditor::minimumZoom, MAX_ZOOM);
                if (zoom != editor.zoom)
                    editor.finishEdit();
                editor.zoom = zoom;
                resizeWorldView(window, view, window.getSize(), editor.zoom);
            }
        }
        if (const auto* move = event->getIf<sf::Event::MouseMoved>())
            updateDrag(move->position);
        if (const auto* release = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (release->button == sf::Mouse::Button::Left)
            {
                updateDrag(release->position);
                if (editor.rectangleBounds())
                    editor.commitRectangle(ui, library.assets);
                else
                    editor.finishEdit();
            }
        }
        if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (click->button == sf::Mouse::Button::Middle)
            {
                sampleAtPixel(click->position, shiftHeld());
                continue;
            }
            if (click->button != sf::Mouse::Button::Left && click->button != sf::Mouse::Button::Right)
                continue;
            editor.finishEdit();
            if (!window.hasFocus() || !insideWindow(click->position, window.getSize()))
                continue;
            const EditorCommand hit = editorHit(ui, debug, window.getSize(), click->position);
            if (hit != EditorCommand::Canvas)
            {
                if (click->button == sf::Mouse::Button::Left)
                    applyEditorCommand(hit, editor, ui, debug, library);
                continue;
            }
            const auto cell = cellAtPixel(click->position);
            if (cell)
            {
                if (click->button == sf::Mouse::Button::Right)
                    editor.clearDecoration(*cell);
                else if (!shiftHeld() || !editor.beginRectangle(*cell, view, library.assets))
                    editor.pressCell(*cell, click->position, view, ui, library.assets);
                debug.log("Clicked cell: " + std::to_string(cell->x) + ", " + std::to_string(cell->y));
            }
        }
    }
    return focusChanged;
}

sf::Vector2f heldDirection(sf::Keyboard::Key up, sf::Keyboard::Key down,
                           sf::Keyboard::Key left, sf::Keyboard::Key right)
{
    sf::Vector2f direction;
    if (sf::Keyboard::isKeyPressed(up))    direction.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(down))  direction.y += 1.f;
    if (sf::Keyboard::isKeyPressed(left))  direction.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(right)) direction.x += 1.f;
    return direction;
}

/** @brief Walk the engineer and center the camera on his new position each step. */
void updateWorld(
    sf::RenderWindow& window,
    sf::View& view,
    const Grid& grid,
    Engineer* engineer,
    float deltaSeconds,
    bool focusChanged,
    const std::vector<Tile>& tiles,
    bool& followEngineer
)
{
    const float mapSize = static_cast<float>(grid.getPixelSize());
    if (!window.hasFocus() || focusChanged)
    {
        if (engineer)
            engineer->update({}, 0.f, mapSize);
        return;
    }

    using Key = sf::Keyboard::Key;
    const sf::Vector2f movement = heldDirection(Key::W, Key::S, Key::A, Key::D);
    sf::Vector2f pan = heldDirection(Key::Up, Key::Down, Key::Left, Key::Right);
    if (engineer)
    {
        engineer->update(movement, deltaSeconds, mapSize);
        if (movement != sf::Vector2f{})
            followEngineer = true;
        else if (pan != sf::Vector2f{})
            followEngineer = false;
        if (followEngineer)
        {
            const auto feet = engineer->getPosition();
            view.setCenter(mapToView(feet) - sf::Vector2f{0.f, terrainElevationAt(grid, tiles, feet)});
            window.setView(view);
            return;
        }
    }

    if (!engineer && movement != sf::Vector2f{})
        pan = movement;
    if (pan == sf::Vector2f{})
        return;

    const float seconds = std::clamp(deltaSeconds, 0.f, 0.05f);
    view.move(pan.normalized() * CAMERA_SPEED * seconds);
    // Start a free pan on the engineer's height plane, even at a raised map corner.
    const float panHeight = engineer ? terrainElevationAt(grid, tiles, engineer->getPosition()) : 0.f;
    const sf::Vector2f lift{0.f, panHeight};
    sf::Vector2f center = viewToMap(view.getCenter() + lift);
    center.x = std::clamp(center.x, 0.f, mapSize);
    center.y = std::clamp(center.y, 0.f, mapSize);
    view.setCenter(mapToView(center) - lift);
    window.setView(view);
}

void drawFrame(
    sf::RenderWindow& window,
    const Grid& grid,
    const Debug& debug,
    std::optional<sf::Vector2i> hoveredCell,
    const IsometricArt& art,
    const MapEditor& editor,
    const Engineer* engineer,
    const EditorUI& ui,
    sf::Vector2i mousePixels,
    EditorLibrary& library
)
{
    window.clear();
    const auto& selectedAsset = editor.getSelectedAsset();
    const auto paintArea = editor.rectangleBounds();
    drawIsometricMap(window, grid, editor.getTiles(), art, engineer, debug.isGridVisible(), hoveredCell,
                     &library.assets, &selectedAsset, editor.getTool() == TerrainTool::Stack, paintArea);
    debug.draw(window);
    const AssetEntry* selected = selectedAsset.id.empty() ? nullptr : library.assets.index().find(selectedAsset.id);
    ui.draw(window, art, editor.getBrush(), editor.getTool(), debug.isGridVisible(), mousePixels,
            window.hasFocus() && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left),
            selected ? selected->name : std::string_view{}, editor.canUndo(), editor.canRedo(),
            paintArea, selected && selected->groundSurface);
    if (library.browser.isOpen())
        library.browser.draw(window, library.assets.index(), library.assets.art, selectedAsset,
                             library.assets.indexing());
    window.display();
}

/** @brief Keep the view, selected brush and painted tiles alive between frames. */
void runGame(
    sf::RenderWindow& window,
    const Grid& grid,
    Debug& debug,
    const IsometricArt& art,
    const std::filesystem::path& savePath,
    Engineer* engineer
)
{
    const float mapSize = static_cast<float>(grid.getPixelSize());
    MapEditor editor{makeSampleTiles(grid.size), savePath};
    EditorUI ui(debug.getFont());
    EditorLibrary library{AssetLibrary(savePath.parent_path() / "assets"), AssetBrowser(debug.getFont())};
    bool followEngineer = engineer != nullptr;
    sf::View view = window.getView();
    const sf::Vector2f start = engineer ? engineer->getPosition() : sf::Vector2f(mapSize / 2.f, mapSize / 2.f);
    const float startHeight = engineer ? terrainElevationAt(grid, editor.getTiles(), start) : 0.f;
    view.setCenter(mapToView(start) - sf::Vector2f{0.f, startHeight});
    resizeWorldView(window, view, window.getSize(), editor.zoom);
    sf::Clock frameClock;
    if (!debug.getFont())
        window.setTitle("Beltbound | 1-5 Brushes | P Paint  Shift+drag Rectangle  I Pick | B Raise  L Lower  E Erase | Ctrl+Z Undo  Ctrl+Y Redo | F2 Save  F4 Load  F3 Grid");

    while (window.isOpen())
    {
        const bool wasIndexing = library.assets.indexing();
        library.assets.pollIndex();
        if (wasIndexing && !library.assets.indexing())
            editor.finishEdit(); // Newly available stamps can change which cell is visible.
        const float deltaSeconds = frameClock.restart().asSeconds();
        const bool focusChanged = handleEvents(window, view, debug, grid, editor, ui, art, library);
        if (!window.isOpen())
        {
            debug.log("Window closed; exiting game loop.");
            break;
        }

        updateWorld(window, view, grid, engineer, deltaSeconds, focusChanged || library.browser.isOpen(),
                    editor.getTiles(), followEngineer);
        editor.finishIfCameraMoved(view);
        const auto mousePixels = sf::Mouse::getPosition(window);
        const auto projectedPoint = projectedMousePoint(window, mousePixels);
        const bool overUI = library.browser.isOpen() || editorHit(ui, debug, window.getSize(), mousePixels) != EditorCommand::Canvas;
        const auto hoveredCell = overUI ? std::nullopt : pickTerrainCell(grid, editor.getTiles(), projectedPoint, art,
                                                                        &library.assets);
        debug.update(deltaSeconds, mousePixels, viewToMap(projectedPoint), grid, hoveredCell, overUI);
        drawFrame(window, grid, debug, hoveredCell, art, editor, engineer, ui, mousePixels, library);
    }
}

/**
 * @brief Own the window, configured grid, and diagnostics for the game's lifetime.
 * @param argc Number of command-line arguments; checked before reading argv[0].
 * @param argv argv[0] supplies the executable path used to locate bundled assets.
 * @return Zero after the game loop finishes normally.
 */
int main(
    int argc,
    char* argv[]
)
{
    sf::RenderWindow window;
    setupWindow(window);
    Debug debug;
    Grid grid;
    grid.setGridSize(100); // Change the number of cells along each side here.

    std::filesystem::path executableFolder;
    if (argc > 0)
    {
        debug.log("Executable path: " + std::string(argv[0]));
        executableFolder = std::filesystem::absolute(argv[0]).parent_path();
    }
    else
    {
        debug.log("No executable path provided; using current working directory.");
        executableFolder = std::filesystem::current_path();
    }

    const std::filesystem::path savePath = executableFolder / "grass.txt";
    debug.loadFont(executableFolder / "assets/fonts/Tuffy.ttf");

    const auto packFolder = executableFolder / "assets/mythryl-studio/tiny-warfare";
    IsometricArt art;
    if (!loadIsometricArt(art, packFolder))
        debug.log("Tiny Warfare terrain unavailable. Using built-in isometric art.");

    // The starter patch has open grass in its middle cell.
    const float spawn = (grid.size / 2 + 0.5f) * grid.cellSize;
    Engineer engineer({spawn, spawn});
    Engineer* activeEngineer = engineer.load(packFolder) ? &engineer : nullptr;
    if (activeEngineer)
        debug.log("WASD walks the woodland engineer. Use the arrows to pan while standing still.");
    else
        debug.log("Woodland engineer sheets unavailable in " + packFolder.string() + ". WASD or arrows pan the map.");

    debug.log("Started Beltbound. Press F3 to toggle the debug overlay.");
    runGame(window, grid, debug, art, savePath, activeEngineer);

    return 0;
}

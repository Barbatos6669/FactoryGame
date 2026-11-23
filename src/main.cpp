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
#include <filesystem>
#include <string>
#include <vector>

#include "Debug.hpp"
#include "Grid.hpp"
#include "Tile.hpp"



constexpr unsigned int WINDOW_WIDTH = 1920;  ///< Initial window width in pixels.
constexpr unsigned int WINDOW_HEIGHT = 1080; ///< Initial window height in pixels.
constexpr float CAMERA_SPEED = 400.f;       ///< Camera movement in world units per second.

/** @brief Open the game window with a 60 FPS limit and repeated key events disabled. */
void setupWindow(
    sf::RenderWindow& window
)
{
    window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Beltbound");
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
}

struct BrushShortcut
{
    sf::Keyboard::Key key;
    Brush brush;
    const char* label;
};

constexpr BrushShortcut brushShortcuts[] = {
    {sf::Keyboard::Key::Num1, Brush::Grass,  "1 Grass"},
    {sf::Keyboard::Key::Num2, Brush::Dirt,   "2 Dirt"},
    {sf::Keyboard::Key::Num3, Brush::Water,  "3 Water"},
    {sf::Keyboard::Key::Num4, Brush::Rock,   "4 Rock"},
    {sf::Keyboard::Key::Num5, Brush::Flower, "5 Flower"}
};

void showBrushControls(sf::RenderWindow& window, Brush activeBrush)
{
    std::string title = "Beltbound  |";
    for (const BrushShortcut& shortcut : brushShortcuts)
    {
        title += "  ";
        if (shortcut.brush == activeBrush)
            title += std::string("[") + shortcut.label + "]";
        else
            title += shortcut.label;
    }
    title += "  |  Left: paint   Right: clear object   F2: save   F4: load";
    window.setTitle(title);
}

struct TileArt
{
    sf::Texture terrain;
    sf::Texture rock;
    sf::Texture flower;
};

bool loadTileArt(TileArt& art, const std::filesystem::path& tilesetFolder)
{
    sf::Image sheet;
    if (!sheet.loadFromFile(tilesetFolder / "The Ground v1-1 - Bright.png"))
        return false;
    if (sheet.getSize().x < 1312 || sheet.getSize().y < 96 || !art.terrain.loadFromImage(sheet))
        return false;

    // The flower tile at (0,32) repeats the grass at (0,0). Keep only the changed pixels.
    sf::Image flowers({32, 32}, sf::Color::Transparent);
    for (unsigned int y = 0; y < 32; ++y)
    {
        for (unsigned int x = 0; x < 32; ++x)
        {
            const sf::Color pixel = sheet.getPixel({x, y + 32});
            if (pixel != sheet.getPixel({x, y}))
                flowers.setPixel({x, y}, pixel);
        }
    }
    const sf::IntRect stonePile({96, 0}, {32, 32});
    return art.flower.loadFromImage(flowers) &&
           art.rock.loadFromFile(tilesetFolder / "beltbound-resources-tier-1.png", false, stonePile);
}

/** @brief Handle editor input and report focus changes for the camera. */
bool handleEvents(
    sf::RenderWindow& window,
    sf::View& view,
    Debug& debug,
    const Grid& grid,
    std::vector<Tile>& tiles,
    Brush& brush,
    const std::filesystem::path& savePath
)
{
    bool focusChanged = false;
    while (const auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }

        if (event->is<sf::Event::FocusLost>() || event->is<sf::Event::FocusGained>())
        {
            focusChanged = true;
        }

        if (const auto* key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::F2)
            {
                const bool saved = saveTiles(tiles, savePath);
                debug.log((saved ? "Saved tiles to " : "Could not save tiles to ") + savePath.string());
            }
            else if (key->code == sf::Keyboard::Key::F3)
            {
                debug.toggleOverlay();
                debug.toggleGridVisibility();
            }
            else if (key->code == sf::Keyboard::Key::F4)
            {
                const bool loaded = loadTiles(tiles, savePath);
                debug.log((loaded ? "Loaded tiles from " : "Could not load tiles from ") + savePath.string());
            }
            else
            {
                for (const BrushShortcut& shortcut : brushShortcuts)
                {
                    if (key->code == shortcut.key)
                    {
                        brush = shortcut.brush;
                        showBrushControls(window, brush);
                        break;
                    }
                }
            }
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            if (resized->size.x > 0 && resized->size.y > 0)
            {
                view.setSize(sf::Vector2f(resized->size));
                window.setView(view);
            }
        }
        if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (click->button != sf::Mouse::Button::Left && click->button != sf::Mouse::Button::Right)
                continue;
            const auto clickWorld = window.mapPixelToCoords(click->position);
            debug.log("Map click: " + std::to_string(clickWorld.x) + ", " + std::to_string(clickWorld.y));
            if (grid.contains(clickWorld))
            {
                const auto cell = grid.worldToCell(clickWorld);
                if (click->button == sf::Mouse::Button::Left)
                    paintTile(tiles, cell, brush);
                else
                    clearTileObject(tiles, cell);
                debug.log("Clicked cell: " + std::to_string(cell.x) + ", " + std::to_string(cell.y));
            }
        }
    }
    return focusChanged;
}

/**
 * @brief Pan with WASD and apply the bounded world view before mouse conversion.
 * @param window Supplies focus state and receives the updated view.
 * @param view Persistent camera; its center is clamped to [0, grid.getPixelSize()] per axis.
 * @param grid Defines the inclusive camera-center bounds, not viewport bounds.
 * @param deltaSeconds Elapsed frame time in seconds; movement uses at most 0.05 seconds.
 * @param focusChanged Suppresses movement on frames containing a focus change.
 * @note Movement requires focus; opposite keys cancel and diagonals are normalized.
 */
void updateCamera(
    sf::RenderWindow& window,
    sf::View& view,
    const Grid& grid,
    float deltaSeconds,
    bool focusChanged
)
{
    if (!window.hasFocus() || focusChanged)
        return;

    sf::Vector2f direction;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) direction.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) direction.y += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) direction.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) direction.x += 1.f;

    if (direction.lengthSquared() == 0.f)
        return;

    // Limit movement after a pause or unusually long frame.
    const float cameraSeconds = std::clamp(deltaSeconds, 0.f, 0.05f);
    view.move(direction.normalized() * CAMERA_SPEED * cameraSeconds);
    const float mapSize = static_cast<float>(grid.getPixelSize());
    sf::Vector2f center = view.getCenter();
    center.x = std::clamp(center.x, 0.f, mapSize);
    center.y = std::clamp(center.y, 0.f, mapSize);
    view.setCenter(center);
    window.setView(view);
}

/**
 * @brief Draw the hover outline when the mouse lies inside the grid.
 * @param target Draws using its active world view.
 * @param grid Supplies cell bounds and world-to-cell conversion.
 * @param mouseWorld Mouse position already mapped through the world view.
 * @param outline Sprite for the outline tiles.
 */
void drawHover(
    sf::RenderTarget& target,
    const Grid& grid,
    sf::Vector2f mouseWorld,
    sf::Sprite& outline
)
{
    if (!grid.contains(mouseWorld))
        return;

    const auto cell = grid.worldToCell(mouseWorld);
    outline.setPosition({static_cast<float>(cell.x * grid.cellSize),
                         static_cast<float>(cell.y * grid.cellSize)});
    target.draw(outline);
}

/** @brief Draw the terrain first so that any object appears on top of it. */
void drawTiles(sf::RenderTarget& target, const std::vector<Tile>& tiles, int cellSize, const TileArt& art)
{
    sf::Sprite ground(art.terrain);
    sf::Sprite object(art.rock);
    const float scale = static_cast<float>(cellSize) / 32.f;
    ground.setScale({scale, scale});
    object.setScale({scale, scale});

    for (const Tile& tile : tiles)
    {
        const sf::Vector2f position(static_cast<float>(tile.cell.x) * cellSize,
                                    static_cast<float>(tile.cell.y) * cellSize);
        switch (tile.terrain)
        {
            case TerrainType::Grass: ground.setTextureRect({{0, 0}, {32, 32}});    break;
            case TerrainType::Dirt:  ground.setTextureRect({{0, 64}, {32, 32}});   break;
            case TerrainType::Water: ground.setTextureRect({{1280, 0}, {32, 32}}); break;
        }
        ground.setPosition(position);
        target.draw(ground);

        if (tile.object != TileObject::None)
        {
            object.setTexture(tile.object == TileObject::Rock ? art.rock : art.flower);
            object.setPosition(position);
            target.draw(object);
        }
    }
}

void drawFrame(
    sf::RenderWindow& window,
    const Grid& grid,
    const Debug& debug,
    sf::Vector2f mouseWorld,
    const TileArt& art,
    sf::Sprite& outline,
    const std::vector<Tile>& tiles
)
{
    window.clear();
    drawTiles(window, tiles, grid.cellSize, art);
    if (debug.isGridVisible())
    {
        grid.draw(window);
    }

    drawHover(window, grid, mouseWorld, outline);
    debug.draw(window);
    window.display();
}

/** @brief Keep the view, selected brush and painted tiles alive between frames. */
void runGame(
    sf::RenderWindow& window,
    const Grid& grid,
    Debug& debug,
    const TileArt& art,
    sf::Sprite& outline,
    const std::filesystem::path& savePath
)
{
    const float mapSize = static_cast<float>(grid.getPixelSize());
    sf::View view = window.getView();
    view.setCenter({mapSize / 2.f, mapSize / 2.f});
    window.setView(view);
    sf::Clock frameClock;
    std::vector<Tile> tiles = makeSampleTiles(grid.size);
    Brush brush = Brush::Grass;
    showBrushControls(window, brush);

    while (window.isOpen())
    {
        const float deltaSeconds = frameClock.restart().asSeconds();
        const bool focusChanged = handleEvents(window, view, debug, grid, tiles, brush, savePath);
        if (!window.isOpen())
        {
            debug.log("Window closed; exiting game loop.");
            break;
        }

        updateCamera(window, view, grid, deltaSeconds, focusChanged);
        const auto mousePixels = sf::Mouse::getPosition(window);
        const auto mouseWorld = window.mapPixelToCoords(mousePixels);
        debug.update(deltaSeconds, mousePixels, mouseWorld, grid);
        drawFrame(window, grid, debug, mouseWorld, art, outline, tiles);
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

    TileArt art;
    const auto tilesetFolder = executableFolder / "assets/tilesets";
    if (!loadTileArt(art, tilesetFolder))
    {
        debug.log("Could not load tile art from " + tilesetFolder.string());
        return 1;
    }

    const auto uiPath = executableFolder /
        "assets/ui/kenney-pixel-adventure/Tilesheets/Large tiles/Thick outline/tilemap_packed.png";

    sf::Texture uiTexture;
    if (!uiTexture.loadFromFile(uiPath))
    {
        debug.log("Could not load UI texture: " + uiPath.string());
        return 1;
    }
    else
    {
        debug.log("Loaded UI texture: " + uiPath.string());
    }

    const sf::IntRect outlineArea({288, 0}, {32, 32});
    sf::Sprite outline(uiTexture, outlineArea);

    debug.log("Started Beltbound. Press F3 to toggle the debug overlay.");
    runGame(window, grid, debug, art, outline, savePath);

    return 0;
}

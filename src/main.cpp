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
#include <vector>

#include "Debug.hpp"
#include "Grid.hpp"



constexpr unsigned int WINDOW_WIDTH = 1920;  ///< Initial window width in pixels.
constexpr unsigned int WINDOW_HEIGHT = 1080; ///< Initial window height in pixels.
constexpr float CAMERA_SPEED = 400.f;       ///< Camera movement in world units per second.

/** @brief Open the game window with a 60 FPS limit and repeated key events disabled. */
void setupWindow(sf::RenderWindow& window)
{
    window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Beltbound");
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
}

/**
 * @brief Handle window events, F3, and grass painting for one frame.
 * @param window Event source; a close event closes it.
 * @param view Persistent world view, resized without changing its center.
 * @param debug Logs clicks and toggles the overlay and grid.
 * @param grid Supplies bounds and cell indices for clicks.
 * @param grassCells Receives the cell for each valid left click.
 * @return True if a focus-lost or focus-gained event occurred this frame.
 */
bool handleEvents(sf::RenderWindow& window, sf::View& view, Debug& debug,
                  const Grid& grid, std::vector<sf::Vector2i>& grassCells)
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
            if (key->code == sf::Keyboard::Key::F3)
            {
                debug.toggleOverlay();
                debug.toggleGridVisibility();
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
            if (click->button == sf::Mouse::Button::Left)
            {
                const auto clickWorld = window.mapPixelToCoords(click->position); // Map the click position to world coordinates
                debug.log("Map click: " + std::to_string(clickWorld.x) + ", " + std::to_string(clickWorld.y));

                if (grid.contains(clickWorld))
                {
                    const auto cell = grid.worldToCell(clickWorld); // Convert the world position to cell indices
                    grassCells.push_back(cell);
                    debug.log("Clicked cell: " + std::to_string(cell.x) + ", " + std::to_string(cell.y));
                }
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
void updateCamera(sf::RenderWindow& window, sf::View& view, const Grid& grid,
                  float deltaSeconds, bool focusChanged)
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
void drawHover(sf::RenderTarget& target, const Grid& grid, sf::Vector2f mouseWorld, sf::Sprite& outline)
{
    if (!grid.contains(mouseWorld))
        return;

    const auto cell = grid.worldToCell(mouseWorld);
    outline.setPosition({static_cast<float>(cell.x * grid.cellSize),
                         static_cast<float>(cell.y * grid.cellSize)});
    target.draw(outline);
}

/**
 * @brief Draw grass, the optional grid, the hover outline, and diagnostics in that order.
 * @param window Receives the draw calls and presents the frame.
 * @param grid Supplies cell sizes and grid drawing.
 * @param debug Draws diagnostics and supplies the grid visibility setting.
 * @param mouseWorld World position used to locate the hovered cell.
 * @param grass Sprite repositioned for each grass cell.
 * @param outline Sprite repositioned for the hovered cell.
 * @param grassCells Stored grid indices to draw as grass.
 */
void drawFrame(sf::RenderWindow& window, const Grid& grid, const Debug& debug,
               sf::Vector2f mouseWorld, sf::Sprite& grass, sf::Sprite& outline,
               const std::vector<sf::Vector2i>& grassCells)
{
    window.clear();
    for (const auto& cell : grassCells)
    {
        grass.setPosition({static_cast<float>(cell.x * grid.cellSize),
                           static_cast<float>(cell.y * grid.cellSize)});
        window.draw(grass);
    }

    if (debug.isGridVisible())
    {
        grid.draw(window);
    }

    drawHover(window, grid, mouseWorld, outline);
    debug.draw(window);
    window.display();
}

/**
 * @brief Own the frame clock, initially centered view, and grass cells for the run.
 * @param window Remains in use until it is closed.
 * @param grid Grid owned by main() and shared across frames.
 * @param debug Diagnostics owned by main(); updated after camera and mouse conversion.
 * @param grass Sprite for the grass tiles.
 * @param outline Sprite for the outline tiles.
 */
void runGame(sf::RenderWindow& window, const Grid& grid, Debug& debug, sf::Sprite& grass, sf::Sprite& outline)
{
    const float mapSize = static_cast<float>(grid.getPixelSize());
    sf::View view = window.getView();
    view.setCenter({mapSize / 2.f, mapSize / 2.f});
    window.setView(view);
    sf::Clock frameClock;
    std::vector<sf::Vector2i> grassCells;

    const int middleCell = grid.size / 2;
    grassCells.push_back({middleCell, middleCell});

    while (window.isOpen())
    {
        const float deltaSeconds = frameClock.restart().asSeconds();
        const bool focusChanged = handleEvents(window, view, debug, grid, grassCells);
        if (!window.isOpen())
            break;

        updateCamera(window, view, grid, deltaSeconds, focusChanged);
        const auto mousePixels = sf::Mouse::getPosition(window);
        const auto mouseWorld = window.mapPixelToCoords(mousePixels);
        debug.update(deltaSeconds, mousePixels, mouseWorld, grid);
        drawFrame(window, grid, debug, mouseWorld, grass, outline, grassCells);
    }
}

/**
 * @brief Own the window, configured grid, and diagnostics for the game's lifetime.
 * @param argc Number of command-line arguments; checked before reading argv[0].
 * @param argv argv[0] supplies the executable path used to locate bundled assets.
 * @return Zero after the game loop finishes normally.
 */
int main(int argc, char* argv[])
{
    sf::RenderWindow window;
    setupWindow(window);

    Grid grid;
    grid.setGridSize(100); // Change the number of cells along each side here.

    std::filesystem::path executableFolder;
    if (argc > 0)
    {
        executableFolder = std::filesystem::absolute(argv[0]).parent_path();
    }
    else
    {
        executableFolder = std::filesystem::current_path();
    }

    Debug debug;
    debug.loadFont(executableFolder / "assets/fonts/Tuffy.ttf");

    const auto terrainPath = executableFolder / "assets/tilesets/The Ground v1-1 - Bright.png";
    sf::Texture terrainTexture;
    if (!terrainTexture.loadFromFile(terrainPath))
    {
        debug.log("Could not load terrain texture: " + terrainPath.string());
        return 1;
    }
    else
    {
        debug.log("Loaded terrain texture: " + terrainPath.string());
    }

    sf::Sprite grass(terrainTexture, sf::IntRect({0, 0}, {32, 32}));

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
    runGame(window, grid, debug, grass, outline);

    return 0;
}

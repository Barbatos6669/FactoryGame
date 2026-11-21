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
 * @brief Handle close, focus, resize, and F3 events for one frame.
 * @param window Event source; a close event closes it.
 * @param view Persistent world view, resized without changing its center.
 * @param debug Diagnostics whose overlay F3 toggles.
 * @return True if a focus-lost or focus-gained event occurred this frame.
 */
bool handleEvents(sf::RenderWindow& window, sf::View& view, Debug& debug)
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
 * @brief Draw the blue cell highlight only when the mouse lies inside the grid.
 * @param target Draws using its active world view.
 * @param grid Supplies cell bounds and world-to-cell conversion.
 * @param mouseWorld Mouse position already mapped through the world view.
 */
void drawHover(sf::RenderTarget& target, const Grid& grid, sf::Vector2f mouseWorld)
{
    if (!grid.contains(mouseWorld))
        return;

    const auto cell = grid.worldToCell(mouseWorld);
    const float cellPixels = static_cast<float>(grid.cellSize);
    sf::RectangleShape highlight({cellPixels, cellPixels});
    highlight.setPosition({static_cast<float>(cell.x * grid.cellSize),
                           static_cast<float>(cell.y * grid.cellSize)});
    highlight.setFillColor(sf::Color(60, 100, 160));
    target.draw(highlight);
}

/**
 * @brief Clear, draw hover then grid then Debug, and present the frame.
 * @param mouseWorld World position used for hover; grid lines are drawn over its highlight.
 */
void drawFrame(sf::RenderWindow& window, const Grid& grid, const Debug& debug,
               sf::Vector2f mouseWorld)
{
    window.clear();
    drawHover(window, grid, mouseWorld);
    grid.draw(window);
    debug.draw(window);
    window.display();
}

/**
 * @brief Run frames with one persistent, initially grid-centered view and frame clock.
 * @param window Remains in use until it is closed.
 * @param grid Grid owned by main() and shared across frames.
 * @param debug Diagnostics owned by main(); updated after camera and mouse conversion.
 */
void runGame(sf::RenderWindow& window, const Grid& grid, Debug& debug)
{
    const float mapSize = static_cast<float>(grid.getPixelSize());
    sf::View view = window.getView();
    view.setCenter({mapSize / 2.f, mapSize / 2.f});
    window.setView(view);
    sf::Clock frameClock;

    while (window.isOpen())
    {
        const float deltaSeconds = frameClock.restart().asSeconds();
        const bool focusChanged = handleEvents(window, view, debug);
        if (!window.isOpen())
            break;

        updateCamera(window, view, grid, deltaSeconds, focusChanged);
        const auto mousePixels = sf::Mouse::getPosition(window);
        const auto mouseWorld = window.mapPixelToCoords(mousePixels);
        debug.update(deltaSeconds, mousePixels, mouseWorld, grid);
        drawFrame(window, grid, debug, mouseWorld);
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

    const auto executableFolder = argc > 0
        ? std::filesystem::absolute(argv[0]).parent_path()
        : std::filesystem::current_path();

    Debug debug;
    debug.loadFont(executableFolder / "assets/fonts/Tuffy.ttf");
    debug.log("Started Beltbound. Press F3 to toggle the debug overlay.");
    runGame(window, grid, debug);

    return 0;
}

#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <optional>
#include <string>

class Grid;

/**
 * @brief Console logging and a screen-space diagnostic overlay with independent controls.
 * Owns the font used by the temporary text created during each draw call.
 */
class Debug
{
public:
    /**
     * @brief Load the overlay font, logging a failed attempt if console logging is enabled.
     * @param path Font file to open.
     * @return True when loaded; false leaves overlay drawing disabled.
     */
    bool loadFont(const std::filesystem::path& path);
    const sf::Font* getFont() const;
    sf::FloatRect panelBounds(sf::Vector2u windowSize) const;

    /**
     * @brief Enable or disable console logging without changing overlay visibility.
     * @param enabled Whether log() should write to standard output.
     */
    void setConsoleEnabled(bool enabled);

    /**
     * @brief Read the console logging setting.
     * @return True when log() is enabled.
     */
    bool isConsoleEnabled() const;

    /**
     * @brief Write a [Debug]-prefixed message when console logging is enabled.
     * @param message Message to write, followed by a newline.
     */
    void log(const std::string& message) const;

    /** @brief Toggle overlay visibility and log the change through log(). */
    void toggleOverlay();

    /**
     * @brief Read the requested overlay visibility.
     * @return Visibility flag; drawing also requires a successfully loaded font.
     */
    bool isOverlayVisible() const;

    /**
     * @brief Update averaged FPS and rebuild the mouse/grid diagnostic text.
     * FPS sampling uses finite, positive frame times over at least 0.5 seconds.
     * @param deltaSeconds Elapsed frame time in seconds.
     * @param mousePixels Mouse position relative to the window, in pixels.
     * @param mouseWorld Mouse position mapped through the active world view.
     * @param grid Grid dimensions.
     * @param hoveredCell Visible column or empty map cell under the pointer.
     * @param mouseOverUI Display UI in place of a hovered cell when a panel covers the pointer.
     */
    void update(float deltaSeconds, sf::Vector2i mousePixels,
                sf::Vector2f mouseWorld, const Grid& grid, std::optional<sf::Vector2i> hoveredCell,
                bool mouseOverUI);

    /**
     * @brief Draw the visible, loaded overlay at the target's top right in pixel space.
     * Uses the current target size and restores its previous world view afterward.
     * @param target Render target for the overlay.
     */
    void draw(sf::RenderTarget& target) const;

    /**
     * @brief Access the diagnostic text without drawing it.
     * @return Text from the latest update(), or empty before the first update.
     */
    const std::string& getOverlayText() const;

    bool isGridVisible() const;

    void toggleGridVisibility();

private:
    sf::Font font;
    bool fontLoaded = false;
    bool consoleEnabled = true;
    bool showGrid = true;
    bool overlayVisible = true;
    float fpsElapsed = 0.f;
    unsigned int fpsFrames = 0;
    float framesPerSecond = 0.f;
    std::string overlayText;
    sf::FloatRect textBounds;
};

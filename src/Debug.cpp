#include "Debug.hpp"
#include "Grid.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
constexpr float padding = 12.f;
constexpr float margin = 16.f;
constexpr unsigned int textSize = 18;
}

bool Debug::loadFont(const std::filesystem::path& path)
{
    fontLoaded = font.openFromFile(path);
    if (!fontLoaded)
        log("Could not load debug font: " + path.string());
    else if (!overlayText.empty())
        textBounds = sf::Text(font, overlayText, textSize).getLocalBounds();
    return fontLoaded;
}

const sf::Font* Debug::getFont() const
{
    return fontLoaded ? &font : nullptr;
}

sf::FloatRect Debug::panelBounds(sf::Vector2u windowSize) const
{
    if (!fontLoaded || !overlayVisible || !windowSize.x || !windowSize.y || overlayText.empty())
        return {};
    const sf::Vector2f size = textBounds.size + sf::Vector2f{2.f * padding, 2.f * padding};
    return {{static_cast<float>(windowSize.x) - size.x - margin, margin}, size};
}

void Debug::setConsoleEnabled(bool enabled)
{
    consoleEnabled = enabled;
}

bool Debug::isConsoleEnabled() const
{
    return consoleEnabled;
}

void Debug::log(const std::string& message) const
{
    if (consoleEnabled)
        std::cout << "[Debug] " << message << '\n';
}

void Debug::toggleOverlay()
{
    overlayVisible = !overlayVisible;
    log(overlayVisible ? "Overlay shown" : "Overlay hidden");
}

bool Debug::isOverlayVisible() const
{
    return overlayVisible;
}

void Debug::update(float deltaSeconds, sf::Vector2i mousePixels,
                   sf::Vector2f mouseWorld, const Grid& grid, std::optional<sf::Vector2i> hoveredCell,
                   bool mouseOverUI)
{
    if (deltaSeconds > 0.f && std::isfinite(deltaSeconds))
    {
        fpsElapsed += deltaSeconds;
        ++fpsFrames;
        if (fpsElapsed >= 0.5f)
        {
            framesPerSecond = static_cast<float>(fpsFrames) / fpsElapsed;
            fpsElapsed = 0.f;
            fpsFrames = 0;
        }
    }

    std::ostringstream text;
    text << "Beltbound Debug (F3)\n"
         << std::fixed << std::setprecision(1)
         << "FPS: " << framesPerSecond << '\n'
         << "Mouse px: (" << mousePixels.x << ", " << mousePixels.y << ")\n"
         << "Base plane: (" << mouseWorld.x << ", " << mouseWorld.y << ")\n"
         << "Cell: ";

    if (mouseOverUI)
    {
        text << "UI";
    }
    else if (hoveredCell)
    {
        text << '(' << hoveredCell->x << ", " << hoveredCell->y << ')';
    }
    else
    {
        text << "outside-grid";
    }

    text << '\n'
         << "Grid: " << grid.size << " x " << grid.size
         << " / " << grid.cellSize << " px\n"
         << "Map: " << grid.getPixelSize() << " x " << grid.getPixelSize() << " px";
    const std::string updatedText = text.str();
    if (updatedText != overlayText)
    {
        overlayText = updatedText;
        if (fontLoaded)
            textBounds = sf::Text(font, overlayText, textSize).getLocalBounds();
    }
}

void Debug::draw(sf::RenderTarget& target) const
{
    const sf::Vector2u targetSize = target.getSize();
    const sf::FloatRect bounds = panelBounds(targetSize);
    if (bounds.size.x == 0.f)
        return;

    sf::Text text(font, overlayText, textSize);
    text.setFillColor(sf::Color(235, 240, 245));
    sf::RectangleShape panel(bounds.size);
    panel.setPosition(bounds.position);
    panel.setFillColor(sf::Color(18, 22, 29, 235));
    text.setPosition(bounds.position + sf::Vector2f{padding, padding} - textBounds.position);

    // Save by value: setView() would also change a reference returned by getView().
    const sf::View previousView = target.getView();
    const sf::View screenView(sf::FloatRect(
        {0.f, 0.f},
        {static_cast<float>(targetSize.x), static_cast<float>(targetSize.y)}));
    target.setView(screenView);
    target.draw(panel);
    target.draw(text);
    target.setView(previousView);
}

const std::string& Debug::getOverlayText() const
{
    return overlayText;
}

bool Debug::isGridVisible() const
{
    return showGrid;
}

void Debug::toggleGridVisibility()
{
    showGrid = !showGrid;
    log(showGrid ? "Grid shown" : "Grid hidden");
}

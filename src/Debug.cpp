#include "Debug.hpp"
#include "Grid.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

bool Debug::loadFont(const std::filesystem::path& path)
{
    fontLoaded = font.openFromFile(path);
    if (!fontLoaded)
        log("Could not load debug font: " + path.string());
    return fontLoaded;
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
                   sf::Vector2f mouseWorld, const Grid& grid)
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
         << "Mouse world: (" << mouseWorld.x << ", " << mouseWorld.y << ")\n"
         << "Cell: ";

    if (grid.contains(mouseWorld))
    {
        const sf::Vector2i cell = grid.worldToCell(mouseWorld);
        text << '(' << cell.x << ", " << cell.y << ')';
    }
    else
    {
        text << "outside-grid";
    }

    text << '\n'
         << "Grid: " << grid.size << " x " << grid.size
         << " / " << grid.cellSize << " px\n"
         << "Map: " << grid.getPixelSize() << " x " << grid.getPixelSize() << " px";
    overlayText = text.str();
}

void Debug::draw(sf::RenderTarget& target) const
{
    if (!fontLoaded || !overlayVisible)
        return;

    const sf::Vector2u targetSize = target.getSize();
    if (targetSize.x == 0 || targetSize.y == 0)
        return;

    sf::Text text(font, overlayText, 18);
    text.setFillColor(sf::Color(235, 240, 245));
    const sf::FloatRect bounds = text.getLocalBounds();
    const float padding = 12.f;
    const float margin = 16.f;
    const sf::Vector2f panelSize(bounds.size.x + padding * 2.f,
                               bounds.size.y + padding * 2.f);
    const float panelX = static_cast<float>(targetSize.x) - panelSize.x - margin;

    sf::RectangleShape panel(panelSize);
    panel.setPosition({panelX, margin});
    panel.setFillColor(sf::Color(18, 22, 29, 235));
    text.setPosition({panelX + padding - bounds.position.x,
                      margin + padding - bounds.position.y});

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

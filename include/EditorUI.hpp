#pragma once

#include "IsometricArt.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <string>
#include <string_view>

/// Canvas passes input to the world. Panel consumes input in the spaces between buttons.
enum class EditorCommand { Canvas, Panel, Grass, Dirt, Water, Rock, Flower, Paint, Stack, Lower, Save, Load, ToggleGrid, ToggleLibrary, Erase };

enum class TerrainTool { Paint, Stack, Lower, Erase };

EditorCommand editorCommandForKey(sf::Keyboard::Key key);

class EditorUI
{
public:
    /// Borrows the game's font. A missing font leaves the canvas and keyboard controls usable.
    explicit EditorUI(const sf::Font* font);
    EditorCommand hitTest(sf::Vector2u windowSize, sf::Vector2i pixel) const;
    /// Show an action result in the dock footer for three seconds.
    void notify(const std::string& message);
    void draw(sf::RenderTarget& target, const IsometricArt& art, Brush brush, TerrainTool tool,
              bool gridVisible, sf::Vector2i mousePixel, bool leftDown = false,
              std::string_view activeAssetLabel = {}) const;

private:
    const sf::Font* font;
    std::string feedback;
    sf::Clock feedbackClock;
};

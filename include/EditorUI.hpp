#pragma once

#include "EditorHelp.hpp"
#include "IsometricArt.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <optional>
#include <string>
#include <string_view>

/// Canvas passes input to the world. Panel consumes input in the spaces between buttons.
enum class EditorCommand { Canvas, Panel, Grass, Dirt, Water, Rock, Flower, Paint, Stack, Lower, Save, Load, ToggleGrid, ToggleLibrary, Erase, Undo, Redo, ToggleHelp, ToggleDock };

enum class ClipboardHint { Hidden, Selecting, Empty, Ready, OutsideGrid };

EditorCommand editorCommandForKey(sf::Keyboard::Key key);
EditorCommand editorCommandForKey(const sf::Event::KeyPressed& key);

class EditorUI
{
public:
    EditorHelp help;
    /// Borrows the game's font. A missing font leaves the canvas and keyboard controls usable.
    explicit EditorUI(const sf::Font* font);
    EditorCommand hitTest(sf::Vector2u windowSize, sf::Vector2i pixel) const;
    void toggleCompact() { compact = !compact; }
    bool isCompact() const { return compact; }
    /// Show an action result in the dock footer for three seconds.
    void notify(const std::string& message);
    void draw(sf::RenderTarget& target, const IsometricArt& art, Brush brush, TerrainTool tool,
              bool gridVisible, sf::Vector2i mousePixel, bool leftDown = false,
              std::string_view activeAssetLabel = {}, bool canUndo = false, bool canRedo = false,
              std::optional<sf::IntRect> editArea = std::nullopt, bool groundStamp = false,
              ClipboardHint clipboard = ClipboardHint::Hidden) const;

private:
    const sf::Font* font;
    bool compact = false;
    std::string feedback;
    sf::Clock feedbackClock;
};

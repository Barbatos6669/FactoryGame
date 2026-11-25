#pragma once

#include <SFML/System/Vector2.hpp>
#include <optional>

namespace sf
{
class Font;
class RenderTarget;
class Event;
}

enum class EditorHelpPage { Editing, Tools };
enum class EditorHelpHit { None, Editing, Tools, Close };

EditorHelpHit editorHelpHit(sf::Vector2u windowSize, sf::Vector2i pixel);
void drawEditorHelp(sf::RenderTarget& target, const sf::Font& font, EditorHelpPage page, sf::Vector2i mousePixel);

class EditorHelp
{
public:
    void open() { page = EditorHelpPage::Editing; }
    bool isOpen() const { return page.has_value(); }
    void handleEvent(const sf::Event& event, sf::Vector2u windowSize);
    void draw(sf::RenderTarget& target, const sf::Font& font, sf::Vector2i mousePixel) const;

private:
    std::optional<EditorHelpPage> page;
};

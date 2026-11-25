#include "EditorHelp.hpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <iterator>

namespace
{
using Rect = sf::FloatRect;

struct Shortcut
{
    const char* keys;
    const char* action;
};

constexpr Shortcut mapEditing[] = {
    {"Left click / drag", "Use the selected tool"},
    {"Right-click", "Clear the top decoration"},
    {"Shift+drag", "Paint, raise or lower an area"},
    {"F", "Fill matching ground"},
    {"I / Middle-click", "Pick the top brush or stamp"},
    {"Shift+I", "Pick the ground underneath"},
    {"Esc", "Cancel the area or stamp"}
};
constexpr Shortcut copyHistory[] = {
    {"Ctrl+C", "Copy one column"},
    {"Ctrl+drag", "Copy an area"},
    {"Ctrl+V", "Paste at the pointer"},
    {"Ctrl+Z", "Undo the last edit"},
    {"Ctrl+Y / Ctrl+Shift+Z", "Redo the last edit"}
};
constexpr Shortcut brushesTools[] = {
    {"1 / 2 / 3", "Grass / dirt / water"},
    {"4 / 5", "Rock / flower"},
    {"P / B", "Paint / raise"},
    {"L / E", "Lower / erase"}
};
constexpr Shortcut mapView[] = {
    {"F2 / F4", "Save / load the map"},
    {"Tab", "Open the asset library"},
    {"F3", "Toggle grid and diagnostics"},
    {"WASD", "Walk the engineer"},
    {"Arrow keys", "Pan the camera"},
    {"Mouse wheel", "Zoom in or out"},
    {"F6", "Collapse or expand the dock"},
    {"F1 / Esc", "Close shortcuts"}
};

constexpr sf::Color panel{23, 33, 40};
constexpr sf::Color surface{35, 48, 56};
constexpr sf::Color hotSurface{48, 66, 75};
constexpr sf::Color border{69, 89, 97};
constexpr sf::Color paper{236, 242, 241};
constexpr sf::Color muted{171, 188, 192};
constexpr sf::Color cyan{113, 219, 205};
constexpr sf::Color amber{246, 198, 120};

struct HelpLayout
{
    float scale;
    sf::Vector2f screen;
    Rect panel, close, editing, tools;
    float left, right, rowsY, bottom;
    bool compact;

    explicit HelpLayout(sf::Vector2u size)
    {
        const float preferred = size.x >= 2560 && size.y >= 1800 ? 2.f :
                                size.x >= 1920 && size.y >= 1260 ? 1.5f : 1.f;
        scale = std::min({preferred, size.x / 320.f, size.y / 340.f});
        if (scale <= 0.f) return;
        screen = sf::Vector2f(size) / scale;
        const sf::Vector2f extent{std::min(500.f, std::floor(screen.x) - 12.f),
                                  std::min(504.f, std::floor(screen.y) - 12.f)};
        panel = {{std::floor((screen.x - extent.x) / 2.f), std::floor((screen.y - extent.y) / 2.f)}, extent};
        compact = extent.x < 395.f || extent.y < 408.f;
        const float pad = compact ? 10.f : 18.f;
        left = panel.position.x + pad;
        right = panel.position.x + extent.x - pad;
        bottom = panel.position.y + extent.y - pad;
        const float titleHeight = compact ? 27.f : 35.f;
        const float tabHeight = compact ? 23.f : 29.f;
        close = {{right - 57.f, panel.position.y + 7.f}, {57.f, compact ? 23.f : 27.f}};
        const float tabTop = panel.position.y + titleHeight + 9.f;
        const float tabWidth = std::floor((right - left - 4.f) / 2.f);
        editing = {{left, tabTop}, {tabWidth, tabHeight}};
        tools = {{left + tabWidth + 4.f, tabTop}, {right - left - tabWidth - 4.f, tabHeight}};
        rowsY = tabTop + tabHeight + (compact ? 3.f : 6.f);
    }

    EditorHelpHit hit(sf::Vector2i pixel) const
    {
        if (scale <= 0.f) return EditorHelpHit::None;
        const auto point = (sf::Vector2f(pixel) + sf::Vector2f{0.5f, 0.5f}) / scale;
        if (editing.contains(point)) return EditorHelpHit::Editing;
        if (tools.contains(point)) return EditorHelpHit::Tools;
        if (close.contains(point)) return EditorHelpHit::Close;
        return EditorHelpHit::None;
    }
};

void fill(sf::RenderTarget& target, Rect area, sf::Color color)
{
    sf::RectangleShape shape(area.size);
    shape.setPosition(area.position);
    shape.setFillColor(color);
    target.draw(shape);
}

// Anchor text by its ink so shortcut keys and descriptions share a center line.
void textAt(sf::RenderTarget& target, const sf::Font& font, const char* value, unsigned size,
            sf::Vector2f point, sf::Color color, bool bold = false, bool centered = false)
{
    sf::Text text(font, value, size);
    if (bold) text.setStyle(sf::Text::Bold);
    text.setFillColor(color);
    const auto bounds = text.getLocalBounds();
    point -= bounds.position;
    point.y -= bounds.size.y / 2.f;
    if (centered) point.x -= bounds.size.x / 2.f;
    text.setPosition({std::round(point.x), std::round(point.y)});
    target.draw(text);
}
}

EditorHelpHit editorHelpHit(sf::Vector2u size, sf::Vector2i pixel)
{
    return HelpLayout(size).hit(pixel);
}

void drawEditorHelp(sf::RenderTarget& target, const sf::Font& font, EditorHelpPage page, sf::Vector2i mousePixel)
{
    const HelpLayout layout(target.getSize());
    if (layout.scale <= 0.f) return;
    const auto oldView = target.getView();
    target.setView(sf::View(Rect{{0.f, 0.f}, layout.screen}));
    fill(target, {{0.f, 0.f}, layout.screen}, {8, 14, 18, 174});
    fill(target, layout.panel, border);
    fill(target, {layout.panel.position + sf::Vector2f{1, 1}, layout.panel.size - sf::Vector2f{2, 2}}, panel);
    const auto hovered = layout.hit(mousePixel);
    textAt(target, font, "Editor shortcuts", layout.compact ? 17 : 22,
           {layout.left, layout.close.getCenter().y}, paper, true);
    fill(target, layout.close, hovered == EditorHelpHit::Close ? hotSurface : surface);
    textAt(target, font, "Close", 13, layout.close.getCenter(), paper, false, true);

    const auto tab = [&](Rect area, const char* name, EditorHelpPage choice, EditorHelpHit hit) {
        const bool selected = page == choice;
        fill(target, area, hovered == hit || selected ? hotSurface : surface);
        if (selected)
            fill(target, {{area.position.x, area.position.y + area.size.y - 2}, {area.size.x, 2}}, cyan);
        textAt(target, font, name, layout.compact ? 13 : 15, area.getCenter(), selected ? cyan : muted,
               selected, true);
    };
    tab(layout.editing, "Map editing", EditorHelpPage::Editing, EditorHelpHit::Editing);
    tab(layout.tools, "Tools and view", EditorHelpPage::Tools, EditorHelpHit::Tools);

    const auto rows = page == EditorHelpPage::Editing ? std::size(mapEditing) + std::size(copyHistory)
                                                        : std::size(brushesTools) + std::size(mapView);
    const float rowHeight = (layout.bottom - layout.rowsY) / static_cast<float>(rows + 2);
    const float innerWidth = layout.right - layout.left;
    const float actionX = layout.left + std::floor(innerWidth * 0.44f);
    float top = layout.rowsY;
    const auto group = [&](const char* title, const auto& shortcuts) {
        textAt(target, font, title, layout.compact ? 11 : 12,
               {layout.left, top + rowHeight / 2.f}, muted, true);
        top += rowHeight;
        for (const Shortcut& entry : shortcuts)
        {
            fill(target, {{layout.left, top}, {innerWidth, 1}}, surface);
            const float middle = top + rowHeight / 2.f;
            textAt(target, font, entry.keys, layout.compact ? 12 : 14, {layout.left + 3.f, middle}, amber);
            textAt(target, font, entry.action, layout.compact ? 13 : 15, {actionX, middle}, paper);
            top += rowHeight;
        }
    };
    if (page == EditorHelpPage::Editing)
    {
        group("TERRAIN", mapEditing);
        group("COPY AND HISTORY", copyHistory);
    }
    else
    {
        group("BRUSHES AND TOOLS", brushesTools);
        group("MAP AND VIEW", mapView);
    }
    target.setView(oldView);
}

void EditorHelp::handleEvent(const sf::Event& event, sf::Vector2u windowSize)
{
    if (!page) return;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        using Key = sf::Keyboard::Key;
        if (key->code == Key::F1 || key->code == Key::Escape)
            page.reset();
        else if (key->code == Key::Left)
            page = EditorHelpPage::Editing;
        else if (key->code == Key::Right)
            page = EditorHelpPage::Tools;
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (click->button != sf::Mouse::Button::Left) return;
        switch (editorHelpHit(windowSize, click->position))
        {
            case EditorHelpHit::Editing: page = EditorHelpPage::Editing; break;
            case EditorHelpHit::Tools: page = EditorHelpPage::Tools; break;
            case EditorHelpHit::Close: page.reset(); break;
            case EditorHelpHit::None: break;
        }
    }
}

void EditorHelp::draw(sf::RenderTarget& target, const sf::Font& font, sf::Vector2i mousePixel) const
{
    if (page) drawEditorHelp(target, font, *page, mousePixel);
}

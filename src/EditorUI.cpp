#include "EditorUI.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Time.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
using Command = EditorCommand;
using Key = sf::Keyboard::Key;
using Rect = sf::FloatRect;

struct BrushButton
{
    Command command;
    Brush brush;
    const char* name;
    const char* shortcut;
    Key key;
};

constexpr std::array<BrushButton, 5> brushes{{
    {Command::Grass, Brush::Grass, "Grass", "1", Key::Num1},
    {Command::Dirt, Brush::Dirt, "Dirt", "2", Key::Num2},
    {Command::Water, Brush::Water, "Water", "3", Key::Num3},
    {Command::Rock, Brush::Rock, "Rock", "4", Key::Num4},
    {Command::Flower, Brush::Flower, "Flower", "5", Key::Num5}
}};

struct ActionButton
{
    Command command;
    const char* shortcut;
    Key key;
};

// The first group selects how a click edits terrain; the second group operates on the map.
constexpr std::array<ActionButton, 8> actions{{
    {Command::Paint, "P", Key::P}, {Command::Stack, "B", Key::B}, {Command::Lower, "L", Key::L},
    {Command::Erase, "E", Key::E},
    {Command::Save, "F2", Key::F2}, {Command::Load, "F4", Key::F4},
    {Command::ToggleGrid, "F3", Key::F3}, {Command::ToggleLibrary, "Tab", Key::Tab}
}};

constexpr sf::Color panelColor{24, 31, 38};
constexpr sf::Color panelLine{74, 87, 94};
constexpr sf::Color cardColor{38, 48, 56};
constexpr sf::Color hoverColor{54, 67, 75};
constexpr sf::Color pressedColor{64, 75, 81};
constexpr sf::Color borderColor{68, 82, 91};
constexpr sf::Color textColor{242, 245, 241};
constexpr sf::Color quietColor{180, 194, 201};
constexpr sf::Color accent{245, 192, 114};
constexpr sf::Color selectedFill{71, 58, 43};

struct Layout
{
    float scale;
    Rect panel;
    std::array<Rect, brushes.size()> brushCards;
    std::array<Rect, actions.size()> actionCards;
    bool stacked;
    float footerY;
};

// Keep bevels and clickable edges on whole screen pixels at non-integer scales.
void snapToPixels(Rect& area, float scale)
{
    const auto snap = [scale](sf::Vector2f p) {
        return sf::Vector2f{std::round(p.x * scale), std::round(p.y * scale)} / scale;
    };
    const auto end = snap(area.position + area.size);
    area.position = snap(area.position);
    area.size = end - area.position;
}

// Drawing and input use the same rectangles, including immediately after a resize.
Layout makeLayout(sf::Vector2u size)
{
    Layout layout{};
    // Leave room for the title bar and taskbar on 1440p and 2160p displays.
    float preferredScale = 1.f;
    if (size.x >= 2560 && size.y >= 1800)
        preferredScale = 2.f;
    else if (size.x >= 1920 && size.y >= 1260)
        preferredScale = 1.5f;
    layout.scale = std::min({preferredScale, size.x / 320.f, size.y / 340.f});
    if (layout.scale <= 0.f)
        return layout;

    const sf::Vector2f screen = sf::Vector2f(size) / layout.scale;
    layout.stacked = screen.x < 680.f;
    const float margin = layout.stacked ? 6.f : 12.f;
    const float width = std::min(736.f, std::floor(screen.x) - 2.f * margin);
    const float padding = layout.stacked ? 8.f : 12.f;
    const bool narrow = width < 435.f;
    const float actionHeight = narrow ? 35.f : layout.stacked ? 26.f : 23.f;
    const float footerHeight = narrow ? 49.f : 37.f;
    const float height = (layout.stacked ? 82.f + 2.f * actionHeight + 11.f : 140.f) + footerHeight;
    const float left = std::floor((screen.x - width) / 2.f);
    const float top = std::floor(screen.y - margin - height);
    layout.panel = {{left, top}, {width, height}};
    layout.footerY = top + height - footerHeight;

    const float actionWidth = 222.f;
    const float innerWidth = width - 2.f * padding;
    const float paletteWidth = layout.stacked ? innerWidth : innerWidth - actionWidth - 16.f;
    const float gap = 4.f;
    const float groupGap = 10.f;
    const float brushWidth = std::floor((paletteWidth - 3.f * gap - groupGap) / brushes.size());
    float x = left + padding;
    const float rowY = top + (layout.stacked ? 24.f : 31.f);
    for (std::size_t i = 0; i < brushes.size(); ++i)
    {
        layout.brushCards[i] = {{x, rowY}, {brushWidth, layout.stacked ? 53.f : 104.f}};
        x += brushWidth + (i == 2 ? groupGap : gap);
    }

    const float groupLabelWidth = 35.f;
    const float actionLeft = layout.stacked ? left + padding + groupLabelWidth : left + width - padding - actionWidth;
    const float actionTop = layout.stacked ? rowY + 58.f : rowY;
    const float buttonWidth = layout.stacked ? std::floor((innerWidth - groupLabelWidth - 3.f * gap) / 4.f)
                                            : (actionWidth - groupGap) / 2.f;
    for (int i = 0; i < static_cast<int>(actions.size()); ++i)
    {
        const int group = i / 4;
        const int slot = i % 4;
        const int column = layout.stacked ? slot : group;
        const int row = layout.stacked ? group : slot;
        const float columnGap = layout.stacked ? gap : groupGap;
        layout.actionCards[i] = {{actionLeft + column * (buttonWidth + columnGap),
                                  actionTop + row * (actionHeight + gap)}, {buttonWidth, actionHeight}};
    }
    if (layout.scale != std::floor(layout.scale))
    {
        snapToPixels(layout.panel, layout.scale);
        for (Rect& card : layout.brushCards) snapToPixels(card, layout.scale);
        for (Rect& card : layout.actionCards) snapToPixels(card, layout.scale);
    }
    return layout;
}

Command commandAt(const Layout& layout, sf::Vector2i pixel)
{
    if (layout.scale <= 0.f)
        return Command::Canvas;
    // A rasterized pixel is sampled at its center, including at fractional UI scales.
    const sf::Vector2f point = (sf::Vector2f(pixel) + sf::Vector2f{0.5f, 0.5f}) / layout.scale;
    if (!layout.panel.contains(point))
        return Command::Canvas;
    for (std::size_t i = 0; i < brushes.size(); ++i)
        if (layout.brushCards[i].contains(point))
            return brushes[i].command;
    for (std::size_t i = 0; i < actions.size(); ++i)
        if (layout.actionCards[i].contains(point))
            return actions[i].command;
    return Command::Panel;
}

void box(sf::RenderTarget& target, const Rect& area, sf::Color fill,
         sf::Color border = sf::Color::Transparent)
{
    sf::RectangleShape shape(area.size);
    shape.setPosition(area.position);
    shape.setFillColor(fill);
    shape.setOutlineColor(border);
    shape.setOutlineThickness(-1.f);
    target.draw(shape);
}

// Anchor text by its ink bounds so short labels and keycaps align with their icons.
void label(sf::RenderTarget& target, const sf::Font& font, const std::string& value, unsigned int size,
           sf::Vector2f position, sf::Color color, bool centered = false, bool bold = false)
{
    sf::Text text(font, value, size);
    if (bold)
        text.setStyle(sf::Text::Bold);
    text.setFillColor(color);
    const Rect bounds = text.getLocalBounds();
    position -= bounds.position;
    if (centered)
        position.x -= bounds.size.x / 2.f;
    text.setPosition({std::round(position.x), std::round(position.y - bounds.size.y / 2.f)});
    target.draw(text);
}

void brushIcon(sf::RenderTarget& target, const IsometricArt& art, Brush brush, sf::Vector2f center)
{
    // The ground frame includes its side faces. Objects are smaller and get a 2x preview.
    const auto groundFront = center + sf::Vector2f{0.f, 3.f};
    const auto objectFront = center + sf::Vector2f{0.f, 19.f};
    switch (brush)
    {
        case Brush::Grass: drawIsometricGround(target, art, TerrainType::Grass, groundFront, 1.f); break;
        case Brush::Dirt:  drawIsometricGround(target, art, TerrainType::Dirt, groundFront, 1.f); break;
        case Brush::Water: drawIsometricGround(target, art, TerrainType::Water, groundFront, 1.f); break;
        case Brush::Rock:  drawIsometricObject(target, art, TileObject::Rock, objectFront, 2.f); break;
        case Brush::Flower: drawIsometricObject(target, art, TileObject::Flower, objectFront, 2.f); break;
    }
}

const char* actionName(Command command, bool grid)
{
    switch (command)
    {
        case Command::Paint: return "Paint";
        case Command::Stack: return "Raise";
        case Command::Lower: return "Lower";
        case Command::Erase: return "Erase";
        case Command::Save: return "Save";
        case Command::Load: return "Load";
        case Command::ToggleGrid: return grid ? "Grid on" : "Grid off";
        case Command::ToggleLibrary: return "Library";
        default: return "";
    }
}

const char* actionHint(Command command, TerrainTool tool, bool grid)
{
    switch (command)
    {
        case Command::Paint: return "Drag to paint terrain or click to place an object";
        case Command::Stack: return "Add one block of the selected terrain";
        case Command::Lower: return "Remove the top block and keep the base";
        case Command::Erase: return "Erase the top block, including the base";
        case Command::Save: return "Save the map to disk";
        case Command::Load: return "Load the last saved map";
        case Command::ToggleGrid: return grid ? "Hide grid and diagnostics" : "Show grid and diagnostics";
        case Command::ToggleLibrary: return "Browse the asset library";
        default: break;
    }
    switch (tool)
    {
        case TerrainTool::Stack: return "Left raises terrain   /   Right clears an object";
        case TerrainTool::Lower: return "Left removes a block   /   Right clears an object";
        case TerrainTool::Paint: return "Drag terrain   /   Click objects   /   Right clears";
        case TerrainTool::Erase: return "Erase the top block, including the base";
    }
    return "";
}
}

EditorCommand editorCommandForKey(sf::Keyboard::Key key)
{
    for (const auto& button : brushes)
        if (button.key == key)
            return button.command;
    for (const auto& button : actions)
        if (button.key == key)
            return button.command;
    return Command::Canvas;
}

EditorUI::EditorUI(const sf::Font* font) : font(font) {}

EditorCommand EditorUI::hitTest(sf::Vector2u windowSize, sf::Vector2i pixel) const
{
    return font ? commandAt(makeLayout(windowSize), pixel) : Command::Canvas;
}

void EditorUI::notify(const std::string& message)
{
    feedback = message;
    feedbackClock.restart();
}

void EditorUI::draw(sf::RenderTarget& target, const IsometricArt& art, Brush brush, TerrainTool tool,
                    bool gridVisible, sf::Vector2i mousePixel, bool leftDown, std::string_view activeAssetLabel) const
{
    const Layout layout = makeLayout(target.getSize());
    if (!font || layout.scale <= 0.f)
        return;
    const auto hovered = commandAt(layout, mousePixel);
    const sf::View oldView = target.getView();
    target.setView(sf::View(Rect{{0.f, 0.f}, sf::Vector2f(target.getSize()) / layout.scale}));

    box(target, layout.panel, panelColor, panelLine);
    const float headingY = layout.panel.position.y + (layout.stacked ? 13.f : 18.f);
    label(target, *font, "TERRAIN", 11, {layout.brushCards[0].position.x, headingY}, quietColor, false, true);
    label(target, *font, "OBJECTS", 11, {layout.brushCards[3].position.x, headingY}, quietColor, false, true);
    const float labelX = layout.brushCards[0].position.x;
    const auto groupHeading = [&](const char* name, const Rect& first) {
        const sf::Vector2f pos = layout.stacked ? sf::Vector2f{labelX, first.position.y + first.size.y / 2.f}
                                                : sf::Vector2f{first.position.x, headingY};
        label(target, *font, name, 11, pos, quietColor, false, true);
    };
    groupHeading("TOOL", layout.actionCards[0]);
    groupHeading("MAP", layout.actionCards[4]);

    for (std::size_t i = 0; i < brushes.size(); ++i)
    {
        const BrushButton& button = brushes[i];
        const Rect card = layout.brushCards[i];
        const bool selected = tool != TerrainTool::Erase && activeAssetLabel.empty() && brush == button.brush;
        const bool hot = hovered == button.command;
        sf::Color fill = selected ? selectedFill : cardColor;
        if (hot && !selected) fill = leftDown ? pressedColor : hoverColor;
        box(target, card, fill, selected ? accent : (hot ? quietColor : borderColor));
        if (selected)
            box(target, {card.position, {card.size.x, 2.f}}, accent);

        const float center = card.position.x + std::floor(card.size.x / 2.f);
        const float iconY = card.position.y + (layout.stacked ? 19.f : 43.f);
        brushIcon(target, art, button.brush, {center + 6.f, iconY});
        label(target, *font, button.shortcut, 12, card.position + sf::Vector2f{6.f, 11.f},
              selected ? accent : quietColor, false, true);
        label(target, *font, button.name, 15, {center, card.position.y + card.size.y - (layout.stacked ? 11.f : 16.f)},
              selected ? accent : textColor, true, selected);
    }

    for (std::size_t i = 0; i < actions.size(); ++i)
    {
        const ActionButton& button = actions[i];
        const Rect card = layout.actionCards[i];
        const bool hot = hovered == button.command;
        const bool active = (button.command == Command::Paint && tool == TerrainTool::Paint) ||
                            (button.command == Command::Stack && tool == TerrainTool::Stack) ||
                            (button.command == Command::Lower && tool == TerrainTool::Lower) ||
                            (button.command == Command::Erase && tool == TerrainTool::Erase);
        const auto fill = active ? accent : (hot ? (leftDown ? pressedColor : hoverColor) : cardColor);
        box(target, card, fill, active ? accent : (hot ? quietColor : borderColor));
        const std::string name = actionName(button.command, gridVisible);
        const float middleY = card.position.y + std::floor(card.size.y / 2.f);
        if (card.size.y > 30.f)
        {
            const float center = card.position.x + card.size.x / 2.f;
            label(target, *font, name, 14, {center, card.position.y + 10.f}, active ? panelColor : textColor, true, active);
            label(target, *font, button.shortcut, 11, {center, card.position.y + 25.f}, active ? panelColor : quietColor, true);
            continue;
        }
        label(target, *font, name, card.size.x < 90.f ? 14 : 15, {card.position.x + 6.f, middleY},
              active ? panelColor : textColor, false, active);
        const Rect keycap{{card.position.x + card.size.x - 27.f, middleY - 9.f}, {22.f, 18.f}};
        box(target, keycap, panelColor, borderColor);
        label(target, *font, button.shortcut, 11, {keycap.position.x + 11.f, middleY}, quietColor, true);
    }

    const float padding = layout.brushCards[0].position.x - layout.panel.position.x;
    const float textX = layout.panel.position.x + padding;
    box(target, {{textX, layout.footerY}, {layout.panel.size.x - 2.f * padding, 1.f}}, borderColor);
    const float availableWidth = layout.panel.size.x - 2.f * padding;
    const bool showingFeedback = !feedback.empty() && feedbackClock.getElapsedTime().asSeconds() < 3.f;
    std::string footer = showingFeedback ? feedback : actionHint(hovered, tool, gridVisible);
    const bool eraseHint = !showingFeedback && (hovered == Command::Erase ||
        (tool == TerrainTool::Erase && (hovered == Command::Canvas || hovered == Command::Panel)));
    if (eraseHint)
    {
        // Fit the full instruction even when the dock is only 320 logical pixels wide.
        const bool narrow = layout.panel.size.x < 435.f;
        if (narrow)
        {
            label(target, *font, "Click or drag to erase one block per cell.", 12, {textX,layout.footerY+9.f}, quietColor);
            label(target, *font, "Includes the base. Ctrl+Z undoes the stroke.", 12, {textX,layout.footerY+23.f}, quietColor);
            label(target, *font, "Right-click clears only the object.", 12, {textX,layout.footerY+37.f}, quietColor);
        }
        else
        {
            label(target, *font, "Click or drag to erase one block per cell, including the base.", 13,
                  {textX,layout.footerY+10.f}, quietColor);
            label(target, *font, "Ctrl+Z undoes the stroke. Right-click clears only the object.", 13,
                  {textX,layout.footerY+26.f}, quietColor);
        }
        target.setView(oldView);
        return;
    }
    if (!showingFeedback && tool != TerrainTool::Erase && !activeAssetLabel.empty())
        footer = "Stamp   " + std::string(activeAssetLabel);
    else if (!showingFeedback && hovered == Command::ToggleLibrary)
        footer = "Browse the asset library";
    sf::Text measure(*font, footer, 13);
    if (measure.getLocalBounds().size.x > availableWidth)
    {
        while (!footer.empty() && measure.getLocalBounds().size.x > availableWidth)
        {
            footer.pop_back();
            measure.setString(footer + "...");
        }
        footer += "...";
    }
    label(target, *font, footer, 13, {textX, layout.footerY + 17.f}, showingFeedback || (tool != TerrainTool::Erase && !activeAssetLabel.empty()) ? accent : quietColor);
    target.setView(oldView);
}

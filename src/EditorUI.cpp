#include "EditorUI.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
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

// Four tool buttons, four map buttons, then the two history controls.
constexpr std::size_t firstHistoryButton = 8;
constexpr std::array<ActionButton, 10> actions{{
    {Command::Paint, "P", Key::P}, {Command::Stack, "B", Key::B}, {Command::Lower, "L", Key::L},
    {Command::Erase, "E", Key::E},
    {Command::Save, "F2", Key::F2}, {Command::Load, "F4", Key::F4},
    {Command::ToggleGrid, "F3", Key::F3}, {Command::ToggleLibrary, "Tab", Key::Tab},
    {Command::Undo, "Ctrl+Z", Key::Unknown}, {Command::Redo, "Ctrl+Y", Key::Unknown}
}};

constexpr sf::Color panelColor{23,35,44};
constexpr sf::Color headerColor{32,49,59};
constexpr sf::Color panelLine{82,105,114};
constexpr sf::Color railColor{17,28,36};
constexpr sf::Color cardColor{39,57,67};
constexpr sf::Color hoverColor{53,76,87};
constexpr sf::Color pressedColor{67,94,104};
constexpr sf::Color borderColor{63,83,93};
constexpr sf::Color textColor{236,244,243};
constexpr sf::Color quietColor{179,195,199};
constexpr sf::Color disabledText{127,146,153};
constexpr sf::Color cyan{124,228,210};
constexpr sf::Color selectedFill{44,83,84};
constexpr sf::Color amber{255,208,140};

struct Layout
{
    float scale = 0.f;
    Rect panel, header, help, toggle;
    std::array<Rect, brushes.size()> brushCards{};
    std::array<Rect, actions.size()> actionCards{};
    bool stacked = false;
    bool narrow = false;
    bool compact = false;
    float padding = 0.f;
    float footerY = 0.f;
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

// Drawing and hit testing share these coordinates, including immediately after a resize.
Layout makeLayout(sf::Vector2u size, bool compact = false)
{
    Layout layout;
    float preferredScale = 1.f;
    if (size.x >= 2560 && size.y >= 1800) preferredScale = 2.f;
    else if (size.x >= 1920 && size.y >= 1260) preferredScale = 1.5f;
    layout.scale = std::min({preferredScale, size.x / 320.f, size.y / 340.f});
    if (layout.scale <= 0.f) return layout;

    const sf::Vector2f screen = sf::Vector2f(size) / layout.scale;
    layout.compact = compact;
    layout.stacked = screen.x < 710.f;
    const float margin = layout.stacked ? 6.f : 12.f;
    const float width = std::min(compact ? 584.f : 864.f, std::floor(screen.x) - 2.f * margin);
    layout.narrow = width < 460.f;
    layout.padding = layout.stacked ? 8.f : 12.f;
    const float headerHeight = layout.narrow ? 40.f : 36.f;
    const float actionHeight = layout.narrow ? 34.f : layout.stacked ? 24.f : 21.f;
    const float brushHeight = layout.stacked ? 55.f : 4.f * actionHeight + 9.f;
    const float footerHeight = compact ? 24.f : layout.narrow ? 47.f : 40.f;
    const float controlsHeight = compact ? 0.f : 22.f + brushHeight + 6.f +
        (layout.stacked ? 2.f * (actionHeight + 4.f) + 28.f : 0.f);
    const float height = headerHeight + controlsHeight + footerHeight;
    const float left = std::floor((screen.x - width) / 2.f);
    const float top = std::floor(screen.y - margin - height);
    layout.panel = {{left,top},{width,height}};
    layout.header = {{left,top},{width,headerHeight}};
    layout.footerY = top + height - footerHeight;

    const float headerButton = layout.narrow ? 62.f : 92.f;
    const float endX = left + width - layout.padding;
    const float buttonsY = top + std::floor((headerHeight - 24.f) / 2.f);
    layout.toggle = {{endX - headerButton, buttonsY}, {headerButton,24.f}};
    layout.help = {{layout.toggle.position.x - 4.f - headerButton, buttonsY}, {headerButton,24.f}};
    snapToPixels(layout.panel, layout.scale);
    snapToPixels(layout.header, layout.scale);
    snapToPixels(layout.help, layout.scale);
    snapToPixels(layout.toggle, layout.scale);
    if (compact) return layout;

    const float inner = width - 2.f * layout.padding;
    const float startX = left + layout.padding;
    const float rowY = top + headerHeight + 22.f;
    const float paletteWidth = layout.stacked ? inner : inner - 244.f;
    const float brushWidth = std::floor((paletteWidth - 24.f) / brushes.size());
    float x = startX;
    for (std::size_t i = 0; i < brushes.size(); ++i)
    {
        layout.brushCards[i] = {{x,rowY},{brushWidth,brushHeight}};
        x += brushWidth + (i == 2 ? 12.f : 4.f);
    }

    const float actionLeft = layout.stacked ? startX + 40.f : endX - 232.f;
    const float actionTop = layout.stacked ? rowY + brushHeight + 6.f : rowY;
    const float buttonWidth = layout.stacked ? std::floor((endX - actionLeft - 12.f) / 4.f) : 112.f;
    for (int i = 0; i < static_cast<int>(firstHistoryButton); ++i)
    {
        const int col = layout.stacked ? i % 4 : i / 4;
        const int row = layout.stacked ? i / 4 : i % 4;
        const float actionX = actionLeft + col * (buttonWidth + (layout.stacked ? 4.f : 8.f));
        layout.actionCards[i] = {{actionX,actionTop + row * (actionHeight + (layout.stacked ? 4.f : 3.f))},
                                 {buttonWidth,actionHeight}};
    }
    const float historyWidth = layout.stacked ? 2.f * buttonWidth + 4.f : 98.f;
    const float historyLeft = layout.stacked ? actionLeft : endX - 2.f * historyWidth - 4.f;
    const float historyTop = layout.stacked ? actionTop + 2.f * (actionHeight + 4.f) : layout.footerY + 8.f;
    for (std::size_t i = firstHistoryButton; i < actions.size(); ++i)
        layout.actionCards[i] = {{historyLeft + (i-firstHistoryButton)*(historyWidth+4.f),historyTop},{historyWidth,24.f}};
    for (Rect& area : layout.brushCards) snapToPixels(area, layout.scale);
    for (Rect& area : layout.actionCards) snapToPixels(area, layout.scale);
    return layout;
}

Command commandAt(const Layout& layout, sf::Vector2i pixel)
{
    if (layout.scale <= 0.f) return Command::Canvas;
    const sf::Vector2f point = (sf::Vector2f(pixel) + sf::Vector2f{0.5f,0.5f}) / layout.scale;
    if (!layout.panel.contains(point)) return Command::Canvas;
    if (layout.toggle.contains(point)) return Command::ToggleDock;
    if (layout.help.contains(point)) return Command::ToggleHelp;
    if (!layout.compact)
    {
        for (std::size_t i = 0; i < brushes.size(); ++i)
            if (layout.brushCards[i].contains(point)) return brushes[i].command;
        for (std::size_t i = 0; i < actions.size(); ++i)
            if (layout.actionCards[i].contains(point)) return actions[i].command;
    }
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
           sf::Vector2f position, sf::Color color, bool centered = false, bool bold = false, float maxWidth = 0.f)
{
    const sf::String full = sf::String::fromUtf8(value.begin(), value.end());
    sf::Text text(font, full, size);
    if (bold)
        text.setStyle(sf::Text::Bold);
    if (maxWidth > 0.f && text.getLocalBounds().size.x > maxWidth)
    {
        const sf::String ellipsis = "...";
        text.setString(ellipsis);
        if (text.getLocalBounds().size.x > maxWidth) return;
        std::size_t low = 0, high = full.getSize();
        while (low < high)
        {
            const auto middle = (low + high + 1) / 2;
            text.setString(full.substring(0, middle) + ellipsis);
            if (text.getLocalBounds().size.x <= maxWidth) low = middle;
            else high = middle - 1;
        }
        text.setString(full.substring(0, low) + ellipsis);
    }
    text.setFillColor(color);
    const Rect bounds = text.getLocalBounds();
    position -= bounds.position;
    if (centered)
        position.x -= bounds.size.x / 2.f;
    text.setPosition({std::round(position.x), std::round(position.y - bounds.size.y / 2.f)});
    target.draw(text);
}

void brushIcon(sf::RenderTarget& target, const IsometricArt& art, Brush brush, sf::Vector2f center, float scale)
{
    TerrainType terrain = TerrainType::Grass;
    if (brush == Brush::Dirt) terrain = TerrainType::Dirt;
    else if (brush == Brush::Water) terrain = TerrainType::Water;
    const sf::Vector2f front = center + sf::Vector2f{0.f,4.f*scale};
    drawIsometricGround(target, art, terrain, front, scale);
    if (brush == Brush::Rock)
        drawIsometricObject(target, art, TileObject::Rock, front, scale);
    else if (brush == Brush::Flower)
        drawIsometricObject(target, art, TileObject::Flower, front, scale);
}

Command toolCommand(TerrainTool tool)
{
    switch (tool)
    {
        case TerrainTool::Paint: return Command::Paint;
        case TerrainTool::Stack: return Command::Stack;
        case TerrainTool::Lower: return Command::Lower;
        case TerrainTool::Erase: return Command::Erase;
    }
    return Command::Paint;
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
        case Command::Undo: return "Undo";
        case Command::Redo: return "Redo";
        default: return "";
    }
}

const char* actionHint(Command command, TerrainTool tool, bool grid, bool canUndo, bool canRedo)
{
    switch (command)
    {
        case Command::Grass: return "Paint grass on the exposed surface";
        case Command::Dirt: return "Paint dirt on the exposed surface";
        case Command::Water: return "Paint water on the exposed surface";
        case Command::Rock: return "Place a rock on the exposed surface";
        case Command::Flower: return "Place flowers on the exposed surface";
        case Command::ToggleHelp: return "Show the editor shortcuts";
        case Command::Paint: return "Drag to paint terrain or click to place an object";
        case Command::Stack: return "Raise a cell or Shift+drag a rectangle";
        case Command::Lower: return "Lower a cell or Shift+drag a rectangle";
        case Command::Erase: return "Erase the top block, including the base";
        case Command::Save: return "Save the map to disk";
        case Command::Load: return "Load the last saved map";
        case Command::ToggleGrid: return grid ? "Hide grid and diagnostics" : "Show grid and diagnostics";
        case Command::ToggleLibrary: return "Browse the asset library";
        case Command::Undo: return canUndo ? "Undo the last edit or brush stroke" : "Nothing to undo";
        case Command::Redo: return canRedo ? "Reapply the last undone edit" : "Nothing to redo";
        default: break;
    }
    switch (tool)
    {
        case TerrainTool::Stack: return "Left raises terrain   /   Right clears an object";
        case TerrainTool::Lower: return "Left removes a block   /   Right clears an object";
        case TerrainTool::Paint: return "Drag to paint   /   Ctrl+drag to copy   /   Right-click clears";
        case TerrainTool::Erase: return "Erase the top block, including the base";
    }
    return "";
}

struct DockHint
{
    std::string title;
    std::string detail;
    std::string extra;
    sf::Color color = textColor;
};

DockHint contextHint(Command hovered, TerrainTool tool, bool gridVisible, bool canUndo, bool canRedo,
                     bool terrainPaint, std::optional<sf::IntRect> area, ClipboardHint clipboard,
                     const std::string& feedback, bool narrow, bool compact)
{
    if (area)
    {
        const char* verb = actionName(toolCommand(tool), gridVisible);
        const char* release = "Release to paint";
        if (clipboard == ClipboardHint::Selecting) { verb = "Copy"; release = "Release to copy"; }
        else if (tool == TerrainTool::Stack) release = "Release to raise";
        else if (tool == TerrainTool::Lower) release = "Release to lower";
        std::string title = std::string(verb) + "   " + std::to_string(area->size.x) + " x " +
                            std::to_string(area->size.y) + " cells";
        if (clipboard != ClipboardHint::Selecting && (tool == TerrainTool::Stack || tool == TerrainTool::Lower))
            title += "   /   Dash = skipped";
        return {title, std::string(release) + "   /   Esc to cancel", {}, amber};
    }
    if (clipboard == ClipboardHint::OutsideGrid)
        return {"Move the whole patch inside the grid", "Ctrl+C copy cell   /   Ctrl+drag copy area", {}, amber};
    if (!feedback.empty()) return {feedback, {}, {}, amber};
    if (clipboard == ClipboardHint::Empty || clipboard == ClipboardHint::Ready)
        return {clipboard == ClipboardHint::Ready ? "Ctrl+V pastes the preview here" : "Copy terrain and objects",
                "Ctrl+C copy cell   /   Ctrl+drag copy area", {}, cyan};
    if (hovered == Command::ToggleDock)
        return {compact ? "Expand to show brushes and tools" : "Use the compact dock to see more map", {}, {}};
    if (hovered == Command::Erase || (tool == TerrainTool::Erase && (hovered == Command::Canvas || hovered == Command::Panel)))
    {
        if (narrow)
            return {"Click or drag to erase one block per cell", "Includes the base. Ctrl+Z undoes the stroke.",
                    "Right-click clears only the object"};
        return {"Drag to erase one block per cell, including its base",
                "Ctrl+Z undoes the stroke   /   Right-click clears the object", {}};
    }

    DockHint hint{actionHint(hovered, tool, gridVisible, canUndo, canRedo), {}, {}};
    const bool onSurface = hovered == Command::Canvas || hovered == Command::Panel;
    if (tool == TerrainTool::Paint && (onSurface || hovered == Command::Paint))
    {
        if (!terrainPaint)
            hint.title = "Click to place   /   Right-click clears";
        else if (narrow)
            hint.title = "Drag to paint   /   Ctrl+drag to copy";
        hint.detail = "I pick   /   Shift+I pick ground";
        if (terrainPaint)
        {
            if (narrow) hint.extra = hint.detail;
            hint.detail = narrow ? "F fill   /   Shift+drag rectangle" :
                "F fill   /   Shift+drag rectangle   /   I pick   /   Shift+I ground";
        }
    }
    else if (onSurface && (tool == TerrainTool::Stack || tool == TerrainTool::Lower))
    {
        if (narrow)
            hint.title = tool == TerrainTool::Stack ? "Click to raise   /   Right-click clears" : "Click to lower   /   Right-click clears";
        hint.detail = tool == TerrainTool::Stack ? "Shift+drag to raise a rectangle" : "Shift+drag to lower a rectangle";
    }
    return hint;
}

// The shortcut chip stays visible even on the active tool. Narrow buttons stack their two labels.
void drawButton(sf::RenderTarget& target, const sf::Font& font, Rect card, const std::string& name,
                const char* shortcut, bool active, bool hot, bool pressed, bool disabled,
                bool header = false, bool shortHeader = false)
{
    sf::Color fill = header ? headerColor : cardColor;
    if (disabled) fill = railColor;
    else if (hot) fill = pressed ? pressedColor : hoverColor;
    if (active) fill = selectedFill;
    const sf::Color ink = disabled ? disabledText : active ? cyan : textColor;
    box(target, card, fill, active ? cyan : (hot && !disabled ? quietColor : borderColor));
    if (shortHeader)
    {
        label(target, font, name, 13, card.getCenter(), ink, true, true);
        return;
    }
    if (card.size.y > 30.f)
    {
        const float center = card.getCenter().x;
        label(target, font, name, 14, {center,card.position.y+10.f}, ink, true, active);
        label(target, font, shortcut, 11, {center,card.position.y+24.f}, disabled ? disabledText : quietColor, true);
        return;
    }
    const float keyWidth = std::string_view(shortcut).size() > 3 ? 39.f : 22.f;
    const float middle = card.getCenter().y;
    const Rect cap{{card.position.x+card.size.x-keyWidth-4.f,middle-7.f},{keyWidth,14.f}};
    label(target, font, name, header ? 13 : 14, {card.position.x+6.f,middle}, ink, false, active,
          cap.position.x-card.position.x-9.f);
    box(target, cap, railColor);
    label(target, font, shortcut, 11, cap.getCenter(), disabled ? disabledText : quietColor, true);
}
}
EditorCommand editorCommandForKey(sf::Keyboard::Key key)
{
    if (key == Key::F1) return Command::ToggleHelp;
    if (key == Key::F6) return Command::ToggleDock;
    if (key == Key::Unknown)
        return Command::Canvas;
    for (const auto& button : brushes)
        if (button.key == key)
            return button.command;
    for (const auto& button : actions)
        if (button.key == key)
            return button.command;
    return Command::Canvas;
}

EditorCommand editorCommandForKey(const sf::Event::KeyPressed& key)
{
    if (key.control && !key.alt && !key.system)
    {
        if (key.code == Key::Z)
            return key.shift ? Command::Redo : Command::Undo;
        if (key.code == Key::Y && !key.shift)
            return Command::Redo;
    }
    return editorCommandForKey(key.code);
}

EditorUI::EditorUI(const sf::Font* font) : font(font) {}

EditorCommand EditorUI::hitTest(sf::Vector2u windowSize, sf::Vector2i pixel) const
{
    return font ? commandAt(makeLayout(windowSize, compact), pixel) : Command::Canvas;
}

void EditorUI::notify(const std::string& message)
{
    feedback = message;
    feedbackClock.restart();
}

void EditorUI::draw(sf::RenderTarget& target, const IsometricArt& art, Brush brush, TerrainTool tool,
                    bool gridVisible, sf::Vector2i mousePixel, bool leftDown, std::string_view activeAssetLabel,
                    bool canUndo, bool canRedo, std::optional<sf::IntRect> editArea, bool groundStamp,
                    ClipboardHint clipboard) const
{
    const Layout layout = makeLayout(target.getSize(), compact);
    if (!font || layout.scale <= 0.f) return;
    const auto hovered = commandAt(layout, mousePixel);
    const auto currentTool = toolCommand(tool);
    const auto& material = brushes[static_cast<std::size_t>(brush)];
    const bool usesBrush = tool == TerrainTool::Paint || tool == TerrainTool::Stack;
    const auto oldView = target.getView();
    target.setView(sf::View(Rect{{0.f,0.f},sf::Vector2f(target.getSize())/layout.scale}));

    box(target, layout.panel, panelColor, panelLine);
    box(target, {{layout.panel.position.x+1.f,layout.footerY},
                 {layout.panel.size.x-2.f, layout.panel.position.y+layout.panel.size.y-layout.footerY-1.f}}, railColor);
    box(target, layout.header, headerColor, panelLine);

    const float textX = layout.panel.position.x + layout.padding;
    const float middleY = layout.header.getCenter().y;
    sf::ConvexShape layer(4);
    layer.setPoint(0,{0.f,5.f}); layer.setPoint(1,{10.f,0.f});
    layer.setPoint(2,{20.f,5.f}); layer.setPoint(3,{10.f,10.f});
    layer.setPosition({textX,middleY-2.f});
    layer.setFillColor({63,133,129}); target.draw(layer);
    layer.move({0.f,-5.f}); layer.setFillColor(cyan); target.draw(layer);

    std::string selection = actionName(currentTool, gridVisible);
    if (usesBrush)
        selection += "  /  " + (activeAssetLabel.empty() ? std::string(material.name) : std::string(activeAssetLabel));
    const float nameX = textX + 28.f;
    if (layout.narrow)
    {
        label(target, *font, "MAP EDITOR", 10, {nameX,middleY-9.f}, quietColor, false, true);
        label(target, *font, selection, 15, {nameX,middleY+7.f}, cyan, false, true, layout.help.position.x-nameX-5.f);
    }
    else
    {
        label(target, *font, "Map editor", 18, {nameX,middleY}, textColor, false, true);
        const float dividerX = nameX + 94.f;
        box(target, {{dividerX,middleY-8.f},{1.f,16.f}}, panelLine);
        label(target, *font, selection, 15, {dividerX+12.f,middleY}, cyan, false, true,
              layout.help.position.x-dividerX-18.f);
    }
    drawButton(target, *font, layout.help, "Shortcuts", "F1", false, hovered == Command::ToggleHelp,
               leftDown, false, true, layout.narrow);
    drawButton(target, *font, layout.toggle, compact ? "Expand" : "Compact", "F6", false,
               hovered == Command::ToggleDock, leftDown, false, true, layout.narrow);

    const bool terrainPaint = activeAssetLabel.empty() ? brush != Brush::Rock && brush != Brush::Flower : groundStamp;
    const auto hint = contextHint(hovered, tool, gridVisible, canUndo, canRedo, terrainPaint, editArea, clipboard,
                                  feedbackClock.getElapsedTime().asSeconds() < 3.f ? feedback : "",
                                  layout.narrow, compact);
    if (compact)
    {
        std::string status = hint.title;
        if (editArea) status += "   /   Esc cancels";
        label(target, *font, status, 13, {textX,layout.footerY+12.f}, hint.color, false, false,
              layout.panel.size.x-2.f*layout.padding);
        target.setView(oldView);
        return;
    }

    const float headingY = layout.brushCards[0].position.y - 10.f;
    label(target, *font, "TERRAIN", 11, {layout.brushCards[0].position.x,headingY}, quietColor, false, true);
    label(target, *font, "OBJECTS", 11, {layout.brushCards[3].position.x,headingY}, quietColor, false, true);
    const auto groupHeading = [&](const char* name, const Rect& first) {
        const auto pos = layout.stacked ? sf::Vector2f{textX,first.getCenter().y} : sf::Vector2f{first.position.x,headingY};
        label(target, *font, name, 11, pos, quietColor, false, true);
    };
    groupHeading("TOOLS",layout.actionCards[0]);
    groupHeading("MAP",layout.actionCards[4]);
    if (layout.stacked) groupHeading("EDIT",layout.actionCards[firstHistoryButton]);

    for (std::size_t i = 0; i < brushes.size(); ++i)
    {
        const auto& button = brushes[i];
        const Rect card = layout.brushCards[i];
        const bool selected = usesBrush && activeAssetLabel.empty() && brush == button.brush;
        const bool hot = hovered == button.command;
        sf::Color fill = selected ? selectedFill : cardColor;
        if (hot) fill = leftDown ? pressedColor : hoverColor;
        box(target, card, fill, selected ? cyan : hot ? quietColor : borderColor);
        const float captionHeight = layout.stacked ? 17.f : 21.f;
        const float captionY = card.position.y + card.size.y - captionHeight - 1.f;
        box(target, {{card.position.x+1.f,captionY},{card.size.x-2.f,captionHeight}},
            selected ? sf::Color{42,73,75} : railColor);
        const float iconScale = layout.stacked ? 1.f : card.size.x >= 75.f ? 2.f : 1.5f;
        brushIcon(target, art, button.brush, {card.getCenter().x,(card.position.y+captionY)/2.f+1.f},iconScale);
        const Rect keycap{card.position+sf::Vector2f{3.f,3.f},{13.f,13.f}};
        box(target,keycap,selected ? cyan : railColor);
        label(target,*font,button.shortcut,10,keycap.getCenter(),selected ? railColor : quietColor,true,true);
        label(target,*font,button.name,layout.stacked ? 13 : 15,{card.getCenter().x,captionY+captionHeight/2.f},
              selected ? cyan : textColor,true,selected);
    }
    for (std::size_t i = 0; i < actions.size(); ++i)
    {
        const auto& button = actions[i];
        const bool disabled = (button.command == Command::Undo && !canUndo) ||
                              (button.command == Command::Redo && !canRedo);
        drawButton(target,*font,layout.actionCards[i],actionName(button.command, gridVisible),button.shortcut,
                   currentTool == button.command,hovered == button.command && !disabled,leftDown,disabled);
    }
    const float right = layout.stacked ? layout.panel.position.x + layout.panel.size.x - layout.padding
                                       : layout.actionCards[firstHistoryButton].position.x - 6.f;
    const float availableWidth = right - textX;
    const float startY = layout.footerY + (hint.detail.empty() ? 19.f : 11.f);
    label(target,*font,hint.title,14,{textX,startY},hint.color,false,true,availableWidth);
    if (!hint.detail.empty())
        label(target,*font,hint.detail,13,{textX,layout.footerY+27.f},quietColor,false,false,availableWidth);
    if (!hint.extra.empty())
        label(target,*font,hint.extra,12,{textX,layout.footerY+40.f},quietColor,false,false,availableWidth);
    target.setView(oldView);
}

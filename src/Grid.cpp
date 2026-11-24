#include "Grid.hpp"
#include "Projection.hpp"
#include <SFML/Graphics/VertexArray.hpp>

namespace
{
sf::Vertex isoVertex(float x, float y, sf::Color color)
{
    return {mapToView({x, y}), color};
}

void outlineDiamond(sf::RenderTarget& target, float x, float y, float side, sf::Color color, float elevation = 0.f)
{
    sf::Vertex outline[] = {isoVertex(x, y, color), isoVertex(x + side, y, color),
                                  isoVertex(x + side, y + side, color), isoVertex(x, y + side, color),
                                  isoVertex(x, y, color)};
    for (auto& point : outline)
        point.position.y -= elevation;
    target.draw(outline, 5, sf::PrimitiveType::LineStrip);
}
}

int Grid::getPixelSize() const
{
    return size * cellSize;
}

void Grid::setGridSize(int newSize)
{
    size = newSize;
}

bool Grid::contains(sf::Vector2f worldPosition) const
{
    if (size <= 0 || cellSize <= 0)
        return false;

    const float mapSize = static_cast<float>(getPixelSize());
    return worldPosition.x >= 0.f && worldPosition.x < mapSize &&
           worldPosition.y >= 0.f && worldPosition.y < mapSize;
}

sf::Vector2i Grid::worldToCell(sf::Vector2f worldPosition) const
{
    return {static_cast<int>(worldPosition.x / cellSize),
            static_cast<int>(worldPosition.y / cellSize)};
}

void Grid::drawCellOutline(sf::RenderTarget& target, sf::Vector2i cell, float elevation, sf::Color color) const
{
    const float side = static_cast<float>(cellSize);
    outlineDiamond(target, cell.x * side, cell.y * side, side, color, elevation);
}

void Grid::draw(sf::RenderTarget& target) const
{
    const float mapSize = static_cast<float>(getPixelSize());
    sf::VertexArray lines(sf::PrimitiveType::Lines);
    for (int i = 1; i < size; ++i)
    {
        const float offset = static_cast<float>(i * cellSize);
        lines.append(isoVertex(offset, 0.f, sf::Color::White));
        lines.append(isoVertex(offset, mapSize, sf::Color::White));
        lines.append(isoVertex(0.f, offset, sf::Color::White));
        lines.append(isoVertex(mapSize, offset, sf::Color::White));
    }
    target.draw(lines);
    outlineDiamond(target, 0.f, 0.f, mapSize, sf::Color::Green);
}

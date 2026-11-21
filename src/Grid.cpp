#include "Grid.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

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

void Grid::draw(sf::RenderTarget& target) const
{
    const float mapSize = static_cast<float>(getPixelSize());
    sf::RectangleShape horizontalLine({mapSize, 1.f});
    sf::RectangleShape verticalLine({1.f, mapSize});
    sf::RectangleShape gridFrame({mapSize, mapSize});

    horizontalLine.setFillColor(sf::Color::White);
    verticalLine.setFillColor(sf::Color::White);

    gridFrame.setFillColor(sf::Color::Transparent);
    gridFrame.setOutlineThickness(1.f);
    gridFrame.setOutlineColor(sf::Color::Green);
    target.draw(gridFrame);

    for (int i = 0; i < size; i++)
    {
        horizontalLine.setPosition({0.f, static_cast<float>(i * cellSize)});
        verticalLine.setPosition({static_cast<float>(i * cellSize), 0.f});
        target.draw(horizontalLine);
        target.draw(verticalLine);
    }
}

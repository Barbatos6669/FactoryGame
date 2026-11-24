#pragma once

#include <SFML/System/Vector2.hpp>

// A 32-unit square on the map projects to a 32-by-16 diamond.
inline sf::Vector2f mapToView(sf::Vector2f point)
{
    return {(point.x - point.y) / 2.f, (point.x + point.y) / 4.f};
}

inline sf::Vector2f viewToMap(sf::Vector2f point)
{
    return {point.x + 2.f * point.y, 2.f * point.y - point.x};
}

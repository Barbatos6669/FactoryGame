#include "IsometricArt.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <initializer_list>
#include <string_view>
#include <utility>

namespace
{
// Front-corner pixel from the pack's sprites.json. The entire 32 x 27 tile includes the side faces.
constexpr sf::Vector2f objectAnchor{17.f, 33.f};

// A 32 x 16 walkable diamond sits above nine pixels of visible earth.
sf::Image makeGround(sf::Color top, sf::Color left, sf::Color right)
{
    sf::Image image(sf::Vector2u(terrainFrameSize), sf::Color::Transparent);
    for (unsigned int x = 0; x < 32; ++x)
    {
        const unsigned int edge = std::min(x, 31u - x);
        const unsigned int backY = 2 + (15 - edge) / 2;
        const unsigned int frontY = 10 + edge / 2;
        for (unsigned int y = backY; y <= frontY; ++y)
            image.setPixel({x, y}, top);
        for (unsigned int y = frontY + 1; y <= frontY + 9; ++y)
            image.setPixel({x, y}, x < 16 ? left : right);
    }
    return image;
}

// Original Beltbound patterns. A period leaves that pixel transparent.
sf::Image makeObject(sf::Vector2u start, std::initializer_list<std::string_view> rows)
{
    sf::Image image({34, 36}, sf::Color::Transparent);
    unsigned int y = start.y;
    for (const auto row : rows)
    {
        for (unsigned int x = 0; x < row.size(); ++x)
        {
            sf::Color ink;
            switch (row[x])
            {
                case 'h': ink = {183, 202, 198}; break; // lit stone
                case 'r': ink = {115, 139, 146}; break;
                case 's': ink = {64, 82, 95}; break;
                case 'p': ink = {230, 132, 163}; break; // pink petals
                case 'v': ink = {189, 156, 236}; break; // violet petals
                case 'y': ink = {255, 217, 114}; break; // pollen
                case 'l': ink = {145, 190, 100}; break; // leaves
                case 'g': ink = {52, 107, 77}; break;   // stems
                default: continue;
            }
            image.setPixel({start.x + x, y}, ink);
        }
        ++y;
    }
    return image;
}

void drawAnchoredSprite(sf::RenderTarget& target, const sf::Texture& texture, sf::Vector2f anchor,
                        sf::Vector2f front, float scale, sf::Color tint = sf::Color::White)
{
    sf::Sprite sprite(texture);
    sprite.setOrigin(anchor);
    sprite.setPosition(front);
    sprite.setScale({scale, scale});
    sprite.setColor(tint);
    target.draw(sprite);
}
}

IsometricArt::IsometricArt() :
    groundImages{makeGround({99, 153, 93}, {65, 104, 74}, {49, 84, 63}),
                 makeGround({172, 127, 83}, {125, 88, 62}, {97, 69, 55}),
                 makeGround({74, 160, 179}, {46, 119, 147}, {40, 97, 128})},
    grass(groundImages[0]), dirt(groundImages[1]), water(groundImages[2]),
    rock(makeObject({10, 21}, {
        "....hhhhh....", "...hhrrrrs...", "..hhrrrrrss..", ".hrrrrrrssss.",
        ".rrrrrssssss.", "rrrrssssssrrs", ".ssssssrrrsss", "...sssssssss."
    })),
    flower(makeObject({10, 16}, {
        "...pp........", "..pppp.......", ".ppypp.......", "..ppp.....v..",
        "...gg....vvv.", "....g...vvyvv", "....g....vvv.", ".ll.g.....g..",
        "..llgg....g..", "....lg...gl..", ".....g..glll.", ".....gggg....",
        "......gg....."
    }))
{
}

bool IsometricArt::groundPixelOpaque(TerrainType terrain, sf::Vector2i texel) const
{
    const auto index = static_cast<std::size_t>(terrain);
    if (index >= groundImages.size() || texel.x < 0 || texel.y < 0 ||
        texel.x >= terrainFrameSize.x || texel.y >= terrainFrameSize.y)
        return false;
    return groundImages[index].getPixel(sf::Vector2u(texel)).a != 0;
}

bool loadIsometricArt(IsometricArt& art, const std::filesystem::path& packFolder)
{
    const auto ground = packFolder / "ground" / "temperate";
    std::error_code error;
    std::array<sf::Image, 3> images;
    sf::Texture grass, dirt, water, rock;
    const auto loadGround = [&](std::size_t index, const std::filesystem::path& file, sf::Texture& texture) {
        auto& image = images[index];
        return image.loadFromFile(ground / file) && image.getSize() == sf::Vector2u(terrainFrameSize) &&
               texture.loadFromImage(image);
    };
    if (!std::filesystem::is_directory(ground, error) ||
        !loadGround(0, "grass_mud/grass_1.png", grass) ||
        !loadGround(1, "grass_mud/mud_1.png", dirt) ||
        !loadGround(2, "water_1.png", water) ||
        !rock.loadFromFile(ground / "nature" / "rock_small.png"))
    {
        art = IsometricArt{};
        return false;
    }
    art.groundImages = std::move(images);
    art.grass = std::move(grass);
    art.dirt = std::move(dirt);
    art.water = std::move(water);
    art.rock = std::move(rock);
    return true;
}

void drawIsometricGround(sf::RenderTarget& target, const IsometricArt& art, TerrainType terrain,
                         sf::Vector2f front, float scale, std::size_t layersAbove)
{
    // Subtle alternating bands keep adjoining blocks of the same material countable.
    const auto tint = layersAbove % 2 == 0 ? sf::Color::White : sf::Color{220, 220, 220};
    switch (terrain)
    {
        case TerrainType::Grass: drawAnchoredSprite(target, art.grass, terrainFrontAnchor, front, scale, tint); break;
        case TerrainType::Dirt:  drawAnchoredSprite(target, art.dirt, terrainFrontAnchor, front, scale, tint); break;
        case TerrainType::Water: drawAnchoredSprite(target, art.water, terrainFrontAnchor, front, scale, tint); break;
    }
}

void drawIsometricObject(sf::RenderTarget& target, const IsometricArt& art, TileObject object,
                         sf::Vector2f front, float scale)
{
    switch (object)
    {
        case TileObject::None:
            break;
        case TileObject::Rock:
            drawAnchoredSprite(target, art.rock, objectAnchor, front, scale);
            break;
        case TileObject::Flower:
            drawAnchoredSprite(target, art.flower, objectAnchor, front, scale);
            break;
    }
}

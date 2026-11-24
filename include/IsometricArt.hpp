#pragma once

#include "Tile.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <array>
#include <cstddef>

/// Each extra block raises a 32-pixel-wide tile by eight screen pixels at 1x.
inline constexpr float terrainLayerHeightRatio = 0.25f;
inline constexpr sf::Vector2f terrainFrontAnchor{16.f, 17.f};
inline constexpr sf::Vector2i terrainFrameSize{32, 27};

struct IsometricArt
{
private:
    // Kept in system memory for pixel-accurate picking without a GPU readback.
    std::array<sf::Image, 3> groundImages;
    friend bool loadIsometricArt(IsometricArt&, const std::filesystem::path&);

public:
    /// Start with original pixel art so every brush works before the optional pack is loaded.
    IsometricArt();
    bool groundPixelOpaque(TerrainType terrain, sf::Vector2i texel) const;
    sf::Texture grass;
    sf::Texture dirt;
    sf::Texture water;
    sf::Texture rock;
    sf::Texture flower;
};

/// Load the four native pack sprites together. Return false and restore the built-in art on failure.
bool loadIsometricArt(IsometricArt& art, const std::filesystem::path& packFolder);

/// Both layers use the front (bottom) corner of the diamond as their world position.
void drawIsometricGround(sf::RenderTarget& target, const IsometricArt& art, TerrainType terrain,
                         sf::Vector2f front, float scale, std::size_t layersAbove = 0);
void drawIsometricObject(sf::RenderTarget& target, const IsometricArt& art, TileObject object,
                         sf::Vector2f front, float scale);

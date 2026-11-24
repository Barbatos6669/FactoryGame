#include "Engineer.hpp"
#include "Projection.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>

namespace
{
// Tiny Warfare v1.0, sprites.json: both clips use the same frame and feet anchor.
constexpr sf::Vector2i frameSize{52, 36};
constexpr sf::Vector2f feet{26.f, 22.f};
constexpr int directionCount = 8;
constexpr float walkSpeed = 96.f;

struct Animation
{
    const char* filename;
    int frames;
    float fps;
};

constexpr Animation idle{"woodland_engineer_idle.png", 4, 6.f};
constexpr Animation walk{"woodland_engineer_walk.png", 8, 12.f};

bool loadAnimation(sf::Texture& texture, const std::filesystem::path& folder, const Animation& clip)
{
    const auto path = folder / clip.filename;
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || !texture.loadFromFile(path))
        return false;
    const sf::Vector2i expectedSize(frameSize.x * clip.frames, frameSize.y * directionCount);
    return texture.getSize() == sf::Vector2u(expectedSize);
}
}

Engineer::Engineer(sf::Vector2f spawn) : position(spawn)
{
}

bool Engineer::load(const std::filesystem::path& packFolder)
{
    const auto folder = packFolder / "units/woodland/engineer/blue";
    return loadAnimation(idleTexture, folder, idle) && loadAnimation(walkTexture, folder, walk);
}

Engineer::Facing Engineer::facingFromStep(sf::Vector2f mapStep)
{
    sf::Vector2f step = mapToView(mapStep);
    // A step along a diamond edge has a 2:1 slope. Choose the nearest of eight sprite rows.
    constexpr float tanHalfSector = 0.41421356f; // tan(22.5 degrees)
    if (std::abs(step.x) < tanHalfSector * std::abs(step.y)) step.x = 0.f;
    if (std::abs(step.y) < tanHalfSector * std::abs(step.x)) step.y = 0.f;
    if (step.y > 0.f)
    {
        if (step.x < 0.f) return Facing::DownLeft;
        if (step.x > 0.f) return Facing::DownRight;
        return Facing::Down;
    }
    if (step.y < 0.f)
    {
        if (step.x < 0.f) return Facing::UpLeft;
        if (step.x > 0.f) return Facing::UpRight;
        return Facing::Up;
    }
    return step.x < 0.f ? Facing::Left : Facing::Right;
}

void Engineer::update(sf::Vector2f input, float deltaSeconds, float mapSize)
{
    const sf::Vector2f previous = position;
    if (input.lengthSquared() > 0.f)
    {
        const float moveSeconds = std::clamp(deltaSeconds, 0.f, 0.05f);
        const sf::Vector2f mapDirection = viewToMap(input);
        position += mapDirection.normalized() * walkSpeed * moveSeconds;
        // A foot on mapSize would be in the cell just beyond the right or bottom edge.
        const float lastPixel = std::max(0.f, mapSize - 1.f);
        position.x = std::clamp(position.x, 0.f, lastPixel);
        position.y = std::clamp(position.y, 0.f, lastPixel);
    }

    const sf::Vector2f step = position - previous;
    const bool moved = step != sf::Vector2f{};
    if (moved)
        lastMapStep = step;
    if (moved != walking)
        frameProgress = 0.f;
    walking = moved;

    const Animation& clip = walking ? walk : idle;
    frameProgress = std::fmod(frameProgress + std::max(0.f, deltaSeconds) * clip.fps,
                              static_cast<float>(clip.frames));
}

sf::Vector2f Engineer::getPosition() const
{
    return position;
}

void Engineer::draw(sf::RenderTarget& target, float elevation) const
{
    const Facing facing = facingFromStep(lastMapStep);
    const sf::Vector2i frameOrigin(static_cast<int>(frameProgress) * frameSize.x,
                                   static_cast<int>(facing) * frameSize.y);
    sf::Sprite sprite(walking ? walkTexture : idleTexture, sf::IntRect(frameOrigin, frameSize));
    sprite.setOrigin(feet);
    sprite.setPosition(mapToView(position) - sf::Vector2f{0.f, elevation});
    target.draw(sprite);
}

#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <filesystem>

class Engineer
{
public:
    explicit Engineer(sf::Vector2f spawn);
    bool load(const std::filesystem::path& packFolder);
    void update(sf::Vector2f input, float deltaSeconds, float mapSize);
    sf::Vector2f getPosition() const;
    void draw(sf::RenderTarget& target, float elevation = 0.f) const;

private:
    // Row order in the woodland engineer's idle and walk sheets.
    enum class Facing { Down, DownLeft, Left, UpLeft, Up, UpRight, Right, DownRight };
    static Facing facingFromStep(sf::Vector2f mapStep);

    sf::Texture idleTexture;
    sf::Texture walkTexture;
    sf::Vector2f position;  ///< Feet in map coordinates.
    sf::Vector2f lastMapStep{0.f, 1.f};
    bool walking = false;
    float frameProgress = 0.f;
};

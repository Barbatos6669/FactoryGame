#pragma once

#include "AssetCatalog.hpp"
#include "Tile.hpp"
#include <SFML/Graphics.hpp>
#include <future>
#include <map>
#include <optional>
#include <tuple>

// Only frames seen in the browser or on the map enter this cache.
class AssetArt
{
public:
    bool drawPreview(sf::RenderTarget& target, const AssetEntry& entry, int row, int frame,
                     sf::FloatRect bounds);
    sf::FloatRect worldBounds(const AssetEntry& entry, int row, sf::Vector2f front, float scale) const;
    bool drawWorld(sf::RenderTarget& target, const AssetEntry& entry, const AssetPlacement& choice,
                   sf::Vector2f front, float scale, sf::Color tint = sf::Color::White);
    // No value means the frame could not be loaded. Terrain can then use its built-in fallback.
    std::optional<bool> hitWorld(const AssetEntry& entry, const AssetPlacement& choice,
                                 sf::Vector2f front, float scale, sf::Vector2f point);

private:
    struct Frame
    {
        sf::Image pixels;
        sf::Texture texture;
        std::size_t lastUse = 0;
        std::size_t bytes = 0;
    };

    using Key = std::tuple<std::string, int, int>;
    std::map<Key, Frame> cached;
    std::map<std::filesystem::path, sf::Font> fonts;
    std::size_t tick = 0;
    std::size_t totalBytes = 0;
    Frame* loadFrame(const AssetEntry& entry, int row, int frame);
};

class AssetLibrary
{
public:
    explicit AssetLibrary(std::filesystem::path folder);
    void requestIndex();
    void pollIndex();
    bool indexing() const;
    const AssetCatalog& index() const;
    const AssetEntry* find(const AssetPlacement& choice, bool groundSurface) const;
    AssetArt art;

private:
    std::filesystem::path folder;
    AssetCatalog empty;
    std::optional<AssetCatalog> catalog;
    std::future<AssetCatalog> pending;
};

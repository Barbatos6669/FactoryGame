#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

enum class AssetKind { Image, Font };

/// Read only the PNG header. Returns zero dimensions if reading fails.
sf::Vector2i assetPngSize(const std::filesystem::path& path);

struct AssetEntry
{
    std::string id, name, category, source, variant;
    std::filesystem::path path;
    sf::Vector2i sheetSize;
    sf::IntRect frame;
    sf::Vector2f anchor;
    int frames = 1;
    int previewFrame = 0;
    std::vector<std::string> rows;
    float fps = 0.f;
    std::vector<sf::Vector2i> footprints;
    std::vector<sf::Vector2f> rowAnchors;
    std::vector<sf::Vector2f> pivots;
    bool rowsAcross = false;
    bool placeable = false;
    bool frontAnchor = false;
    bool groundSurface = false;
    AssetKind kind = AssetKind::Image;

    sf::IntRect frameRect(int row, int column) const;
    sf::Vector2f anchorForRow(int row) const;
};

/// Indexes the application asset directory without decoding images.
class AssetCatalog
{
public:
    AssetCatalog() = default;
    explicit AssetCatalog(const std::filesystem::path& assetsFolder);
    const std::vector<AssetEntry>& entries() const;
    const AssetEntry* find(std::string_view id) const;

private:
    std::vector<AssetEntry> assets;
};

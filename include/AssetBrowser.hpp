#pragma once

#include "Tile.hpp"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <optional>
#include <string>
#include <vector>

class AssetArt;
class AssetCatalog;
struct BrowserLayout;

class AssetBrowser
{
public:
    explicit AssetBrowser(const sf::Font* font);
    bool isOpen() const;
    void toggle();
    bool covers(sf::Vector2u size, sf::Vector2i pixel) const;
    /// Returns a stamp when the player confirms a placeable asset.
    std::optional<AssetPlacement> handleEvent(const sf::Event& event, sf::Vector2u size,
                                               const AssetCatalog& catalog, const AssetPlacement& activeSelection);
    void draw(sf::RenderTarget& target, const AssetCatalog& catalog, AssetArt& art,
              const AssetPlacement& activeSelection, bool indexing = false);

private:
    void prepare(const AssetCatalog& catalog, const AssetPlacement& activeSelection);
    void filter(const AssetCatalog& catalog);
    void inspect(int match, const AssetCatalog& catalog);
    void reveal(const BrowserLayout& layout);
    void scroll(int rows, const BrowserLayout& layout);
    void adjustFrame(const AssetCatalog& catalog, int rowDelta, int frameDelta);
    std::optional<AssetPlacement> confirmStamp(const AssetCatalog& catalog);

    const sf::Font* font;
    const AssetCatalog* indexed = nullptr;
    std::vector<std::string> searchable, categories;
    std::vector<int> matches;
    sf::String query;
    int category = 0;
    int menuFocus = 0;
    int menuStart = 0;
    int firstRow = 0;
    int selected = -1; // Position in matches, independent of scrolling.
    int row = 0;
    int frame = 0;
    sf::Vector2i mouse{-1, -1};
    bool open = false;
    bool opening = false;
    bool focusSearch = false;
    bool replaceQuery = false;
    bool menuOpen = false;
    bool onlyPlaceable = false;
    bool detailPage = false;
};

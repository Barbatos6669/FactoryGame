#include "AssetArt.hpp"
#include <algorithm>
#include <chrono>
#include <utility>

AssetLibrary::AssetLibrary(std::filesystem::path assetsFolder) : folder(std::move(assetsFolder)) {}

void AssetLibrary::requestIndex()
{
    if (!catalog && !pending.valid())
        pending = std::async(std::launch::async, [root = folder] { return AssetCatalog(root); });
}

void AssetLibrary::pollIndex()
{
    if (pending.valid() && pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        catalog.emplace(pending.get());
}

bool AssetLibrary::indexing() const
{
    return pending.valid();
}

const AssetCatalog& AssetLibrary::index() const
{
    return catalog ? *catalog : empty;
}

const AssetEntry* AssetLibrary::find(const AssetPlacement& choice, bool groundSurface) const
{
    if (!catalog || choice.id.empty())
        return nullptr;
    const auto* entry = catalog->find(choice.id);
    if (!entry || !entry->placeable || entry->groundSurface != groundSurface ||
        choice.row < 0 || choice.row >= static_cast<int>(entry->rows.size()) ||
        choice.frame < 0 || choice.frame >= entry->frames)
        return nullptr;
    return entry;
}

AssetArt::Frame* AssetArt::loadFrame(const AssetEntry& entry, int row, int frame)
{
    if (entry.kind != AssetKind::Image || row < 0 || row >= static_cast<int>(entry.rows.size()) ||
        frame < 0 || frame >= entry.frames)
        return nullptr;
    auto [it, fresh] = cached.try_emplace(Key{entry.id, row, frame});
    Frame& result = it->second;
    result.lastUse = ++tick;
    if (fresh)
    {
        constexpr std::size_t budget = 48 * 1024 * 1024;
        constexpr std::size_t maxSheetPixels = 32 * 1024 * 1024;
        const sf::IntRect area = entry.frameRect(row, frame);
        const auto dimensions = assetPngSize(entry.path);
        const auto sheetPixels = static_cast<unsigned long long>(dimensions.x) * dimensions.y;
        const auto framePixels = static_cast<unsigned long long>(area.size.x) * area.size.y;
        sf::Image sheet;
        if (dimensions == entry.sheetSize && sheetPixels > 0 && sheetPixels <= maxSheetPixels &&
            area.size.x > 0 && area.size.y > 0 && framePixels <= budget / 8 && sheet.loadFromFile(entry.path))
        {
            const sf::Vector2i size(sheet.getSize());
            if (area.position.x >= 0 && area.position.y >= 0 &&
                area.size.x <= size.x - area.position.x && area.size.y <= size.y - area.position.y)
            {
                sf::Image crop(sf::Vector2u(area.size), sf::Color::Transparent);
                if (crop.copy(sheet, {}, area) && result.texture.loadFromImage(crop))
                {
                    // The CPU copy supplies click opacity; the GPU copy draws the frame.
                    result.pixels = std::move(crop);
                    result.bytes = static_cast<std::size_t>(8 * framePixels);
                    totalBytes += result.bytes;
                }
            }
        }

        while ((cached.size() > 512 || totalBytes > budget) && cached.size() > 1)
        {
            const auto oldest = std::min_element(cached.begin(), cached.end(), [](const auto& a, const auto& b) {
                return a.second.lastUse < b.second.lastUse;
            });
            totalBytes -= oldest->second.bytes;
            cached.erase(oldest);
        }
    }
    return result.bytes ? &result : nullptr;
}

bool AssetArt::drawPreview(sf::RenderTarget& target, const AssetEntry& entry, int row, int frame,
                           sf::FloatRect bounds)
{
    if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
        return false;
    if (entry.kind == AssetKind::Font)
    {
        auto [it, fresh] = fonts.try_emplace(entry.path);
        if (fresh && !it->second.openFromFile(entry.path))
        {
            fonts.erase(it);
            return false;
        }
        sf::Text sample(it->second, "Aa", 28);
        const auto ink = sample.getLocalBounds();
        const float scale = std::min(bounds.size.x / ink.size.x, bounds.size.y / ink.size.y);
        sample.setScale({scale, scale});
        sample.setOrigin(ink.getCenter());
        sample.setPosition(bounds.getCenter());
        target.draw(sample);
        return true;
    }
    Frame* spriteFrame = loadFrame(entry, row, frame);
    if (!spriteFrame)
        return false;
    const sf::Vector2f size(spriteFrame->texture.getSize());
    const float scale = std::min(bounds.size.x / size.x, bounds.size.y / size.y);
    sf::Sprite sprite(spriteFrame->texture);
    sprite.setScale({scale, scale});
    sprite.setOrigin(size / 2.f);
    sprite.setPosition(bounds.getCenter());
    target.draw(sprite);
    return true;
}

sf::FloatRect AssetArt::worldBounds(const AssetEntry& entry, int row, sf::Vector2f front, float scale) const
{
    // Ground anchors belong on the front corner. Feet and rotation pivots sit on the cell center.
    if (!entry.frontAnchor)
        front.y -= 8.f * scale;
    return {front - entry.anchorForRow(row) * scale, sf::Vector2f(entry.frame.size) * scale};
}

bool AssetArt::drawWorld(sf::RenderTarget& target, const AssetEntry& entry, const AssetPlacement& choice,
                         sf::Vector2f front, float scale, sf::Color tint)
{
    Frame* frame = loadFrame(entry, choice.row, choice.frame);
    if (!frame)
        return false;
    sf::Sprite sprite(frame->texture);
    sprite.setPosition(worldBounds(entry, choice.row, front, scale).position);
    sprite.setScale({scale, scale});
    sprite.setColor(tint);
    target.draw(sprite);
    return true;
}

std::optional<bool> AssetArt::hitWorld(const AssetEntry& entry, const AssetPlacement& choice,
                                       sf::Vector2f front, float scale, sf::Vector2f point)
{
    if (choice.row < 0 || choice.row >= static_cast<int>(entry.rows.size()) ||
        choice.frame < 0 || choice.frame >= entry.frames)
        return std::nullopt;
    const auto bounds = worldBounds(entry, choice.row, front, scale);
    if (!bounds.contains(point))
        return false;
    Frame* frame = loadFrame(entry, choice.row, choice.frame);
    if (!frame)
        return std::nullopt;
    const auto texel = sf::Vector2u((point - bounds.position) / scale);
    return texel.x < frame->pixels.getSize().x && texel.y < frame->pixels.getSize().y &&
           frame->pixels.getPixel(texel).a != 0;
}

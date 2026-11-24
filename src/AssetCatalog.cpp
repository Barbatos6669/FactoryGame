#include "AssetCatalog.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <map>
#include <optional>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace
{
// Reads the local pack's JSON without constructing a second tree of all its metadata.
class SpriteJson
{
public:
    explicit SpriteJson(std::string_view text) : remaining(text) {}

    bool string(std::string& value)
    {
        if (!take('"'))
            return false;
        value.clear();
        while (!remaining.empty())
        {
            const char c = remaining.front();
            remaining.remove_prefix(1);
            if (c == '"')
                return true;
            if (c == '\\')
            {
                if (remaining.empty())
                    return false;
                const char escape = remaining.front();
                remaining.remove_prefix(1);
                switch (escape)
                {
                    case '"': case '\\': case '/': value += escape; break;
                    case 'b': value += '\b'; break;
                    case 'f': value += '\f'; break;
                    case 'n': value += '\n'; break;
                    case 'r': value += '\r'; break;
                    case 't': value += '\t'; break;
                    case 'u': if (!unicode(value)) return false; break;
                    default: return false;
                }
            }
            else
            {
                if (static_cast<unsigned char>(c) < 32)
                    return false;
                value += c;
            }
        }
        return false;
    }

    bool integer(int& value)
    {
        const auto number = token();
        if (number.empty())
            return false;
        const auto end = number.data() + number.size();
        const auto result = std::from_chars(number.data(), end, value);
        return result.ec == std::errc{} && result.ptr == end;
    }

    template <typename ReadField>
    bool object(const ReadField& readField)
    {
        return sequence('{', '}', [&] {
            std::string key;
            return string(key) && take(':') && readField(key);
        });
    }

    template <typename ReadItem>
    bool array(const ReadItem& readItem)
    {
        return sequence('[', ']', readItem);
    }

    bool skip(int depth = 0)
    {
        if (depth == 64)
            return false;
        switch (peek())
        {
            case '{': return object([&](const std::string&) { return skip(depth + 1); });
            case '[': return array([&] { return skip(depth + 1); });
            case '"': {
                std::string ignored;
                return string(ignored);
            }
            default: {
                const auto value = token();
                if (value == "true" || value == "false" || value == "null")
                    return true;
                if (value.empty())
                    return false;
                double number;
                const auto end = value.data() + value.size();
                const auto result = std::from_chars(value.data(), end, number);
                return result.ec == std::errc{} && result.ptr == end;
            }
        }
    }

    char peek()
    {
        const auto first = remaining.find_first_not_of(" \r\n\t");
        remaining.remove_prefix(first == std::string_view::npos ? remaining.size() : first);
        return remaining.empty() ? '\0' : remaining.front();
    }

    bool done()
    {
        peek();
        return remaining.empty();
    }

private:
    std::string_view remaining;

    bool take(char expected)
    {
        if (peek() != expected || remaining.empty())
            return false;
        remaining.remove_prefix(1);
        return true;
    }

    template <typename ReadItem>
    bool sequence(char open, char close, const ReadItem& readItem)
    {
        if (!take(open))
            return false;
        if (take(close))
            return true;
        do
        {
            if (!readItem())
                return false;
            if (take(close))
                return true;
        } while (take(','));
        return false;
    }

    std::string_view token()
    {
        peek();
        const auto stop = remaining.find_first_of(" \t\r\n,]}");
        const auto value = remaining.substr(0, stop);
        remaining.remove_prefix(value.size());
        return value;
    }

    bool unicode(std::string& value)
    {
        unsigned int code;
        if (!hexUnit(code))
            return false;
        if (code >= 0xd800 && code <= 0xdbff)
        {
            unsigned int low;
            if (remaining.substr(0, 2) != "\\u")
                return false;
            remaining.remove_prefix(2);
            if (!hexUnit(low) || low < 0xdc00 || low > 0xdfff)
                return false;
            code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
        }
        else if (code >= 0xdc00 && code <= 0xdfff)
            return false;

        if (code < 0x80)
            value += static_cast<char>(code);
        else
        {
            const int bytes = code < 0x800 ? 2 : (code < 0x10000 ? 3 : 4);
            const int lead[] = {0, 0, 0xc0, 0xe0, 0xf0};
            value += static_cast<char>(lead[bytes] | (code >> (6 * (bytes - 1))));
            for (int i = bytes - 2; i >= 0; --i)
                value += static_cast<char>(0x80 | ((code >> (6 * i)) & 0x3f));
        }
        return true;
    }

    bool hexUnit(unsigned int& code)
    {
        if (remaining.size() < 4)
            return false;
        const auto end = remaining.data() + 4;
        const auto result = std::from_chars(remaining.data(), end, code, 16);
        remaining.remove_prefix(4);
        return result.ec == std::errc{} && result.ptr == end;
    }
};

struct SpriteMetadata
{
    sf::Vector2i size;
    int frames = 0;
    int fps = 0;
    std::vector<std::string> rows;
    std::optional<sf::Vector2f> feet, ground;
    std::vector<sf::Vector2f> pivots;
    std::vector<sf::Vector2i> footprints;
};

bool readPoint(SpriteJson& json, sf::Vector2f& point)
{
    sf::Vector2i position;
    bool xRead = false;
    bool yRead = false;
    const bool success = json.object([&](const std::string& key) {
        if (key == "x")
            return xRead = json.integer(position.x);
        if (key == "y")
            return yRead = json.integer(position.y);
        return json.skip();
    });
    point = sf::Vector2f(position);
    return success && xRead && yRead;
}

bool readPivots(SpriteJson& json, std::vector<sf::Vector2f>& pivots)
{
    const auto readOne = [&] {
        sf::Vector2f pivot;
        if (!readPoint(json, pivot))
            return false;
        pivots.push_back(pivot);
        return true;
    };
    return json.peek() == '[' ? json.array(readOne) : readOne();
}

bool readFootprints(SpriteJson& json, std::vector<sf::Vector2i>& footprints)
{
    // The pack uses [width, height] for shared dimensions or one pair per direction.
    std::vector<int> shared;
    if (!json.array([&] {
        if (json.peek() == '[')
        {
            std::vector<int> dimensions;
            if (!json.array([&] {
                int dimension;
                if (!json.integer(dimension))
                    return false;
                dimensions.push_back(dimension);
                return dimensions.size() <= 2;
            }) || dimensions.size() != 2)
                return false;
            footprints.emplace_back(dimensions[0], dimensions[1]);
        }
        else
        {
            int dimension;
            if (!json.integer(dimension))
                return false;
            shared.push_back(dimension);
        }
        return true;
    }))
        return false;
    if (shared.empty())
        return true;
    if (shared.size() != 2 || !footprints.empty())
        return false;
    footprints.emplace_back(shared[0], shared[1]);
    return true;
}

bool readMetadata(SpriteJson& json, SpriteMetadata& sprite)
{
    return json.object([&](const std::string& key) {
        if (key == "width") return json.integer(sprite.size.x);
        if (key == "height") return json.integer(sprite.size.y);
        if (key == "frames") return json.integer(sprite.frames);
        if (key == "fps") return json.integer(sprite.fps);
        if (key == "rows")
            return json.array([&] { return json.string(sprite.rows.emplace_back()); });
        if (key == "feet") return readPoint(json, sprite.feet.emplace());
        if (key == "ground") return readPoint(json, sprite.ground.emplace());
        if (key == "pivot") return readPivots(json, sprite.pivots);
        if (key == "footprint") return readFootprints(json, sprite.footprints);
        return json.skip();
    });
}

using SpriteManifest = std::map<std::string, SpriteMetadata, std::less<>>;

SpriteManifest readManifest(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return {};
    const auto length = file.tellg();
    if (length <= 0 || length > 64 * 1024 * 1024)
        return {};
    file.seekg(0);
    std::string contents(static_cast<std::size_t>(length), '\0');
    if (!file.read(contents.data(), length))
        return {};

    std::string_view text(contents);
    if (text.substr(0, 3) == "\xef\xbb\xbf")
        text.remove_prefix(3);
    SpriteJson json(text);
    SpriteManifest manifest;
    if (!json.object([&](const std::string& key) {
        if (key != "sprites")
            return json.skip();
        return json.object([&](const std::string& name) {
            SpriteMetadata sprite;
            if (!readMetadata(json, sprite))
                return false;
            manifest.emplace(name, std::move(sprite));
            return true;
        });
    }) || !json.done())
        return {};
    return manifest;
}

constexpr std::string_view tinyWarfare = "mythryl-studio/tiny-warfare/";
constexpr std::string_view engineerId = "characters/beltbound-field-engineer-idle-8dir.png";
constexpr std::string_view oreAtlasId = "tilesets/beltbound-resources-tier-1.png";

bool startsWith(std::string_view word, std::string_view prefix)
{
    return word.substr(0, prefix.size()) == prefix;
}

bool endsWith(std::string_view word, std::string_view suffix)
{
    return word.size() >= suffix.size() && word.substr(word.size() - suffix.size()) == suffix;
}

std::string lowercase(std::string word)
{
    for (char& c : word)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return word;
}

std::string title(std::string_view word)
{
    std::string result;
    bool start = true;
    for (const char c : word)
    {
        if (c == '_' || c == '-' || c == ' ')
        {
            result += ' ';
            start = true;
        }
        else if (c == '/')
        {
            result += " / ";
            start = true;
        }
        else
        {
            result += start && c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
            start = false;
        }
    }
    return result;
}

bool applyMetadata(AssetEntry& asset, const SpriteMetadata& sprite)
{
    if (sprite.size.x <= 0 || sprite.size.x > 16384 || sprite.size.y <= 0 || sprite.size.y > 16384 ||
        sprite.frames <= 0 || sprite.frames > 4096 || sprite.rows.empty() || sprite.rows.size() > 4096)
        return false;

    const int rowCount = static_cast<int>(sprite.rows.size());
    asset.frames = sprite.frames;
    asset.rows = sprite.rows;
    asset.fps = static_cast<float>(sprite.fps);
    asset.sheetSize = {sprite.size.x * sprite.frames, sprite.size.y * rowCount};
    asset.frame = {{0, 0}, sprite.size};
    asset.pivots = sprite.pivots;
    asset.footprints = sprite.footprints;
    if (asset.footprints.size() == 1 && rowCount > 1)
        asset.footprints.resize(rowCount, asset.footprints.front());
    if (asset.footprints.size() != asset.rows.size())
        asset.footprints.clear();

    if (sprite.ground)
    {
        asset.anchor = *sprite.ground;
        asset.frontAnchor = true;
    }
    else if (sprite.feet)
        asset.anchor = *sprite.feet;
    else if (!sprite.pivots.empty())
    {
        asset.anchor = sprite.pivots.front();
        if (sprite.pivots.size() == sprite.rows.size())
            asset.rowAnchors = sprite.pivots;
    }
    else
        asset.anchor = sf::Vector2f(sprite.size) / 2.f;

    return true;
}

std::string paidCategory(std::string_view packPath)
{
    if (endsWith(packPath, "/guide.png")) return "Guides";
    if (packPath.find("/decals/") != std::string_view::npos) return "Decals";
    if (packPath.find("/nature/") != std::string_view::npos) return "Nature";
    if (startsWith(packPath, "ground/")) return "Terrain";
    return title(packPath.substr(0, packPath.find('/')));
}

void inspectPng(AssetEntry& asset, const SpriteManifest& manifest)
{
    asset.rows = {"Still"};
    if (startsWith(asset.id, tinyWarfare))
    {
        const auto packPath = std::string_view(asset.id).substr(tinyWarfare.size());
        const auto parent = packPath.substr(0, packPath.rfind('/'));
        asset.source = "Tiny Warfare";
        asset.category = paidCategory(packPath);
        const auto variantStart = parent.find('/');
        if (variantStart != std::string_view::npos)
            asset.variant = title(parent.substr(variantStart + 1));
        const auto metadata = manifest.find(packPath);
        if (metadata != manifest.end() && applyMetadata(asset, metadata->second))
        {
            asset.placeable = packPath.find("/mask/") == std::string_view::npos;
            asset.groundSurface = asset.placeable && asset.category == "Terrain" &&
                                  asset.frame.size == sf::Vector2i{32, 27} &&
                                  asset.frontAnchor && asset.anchor == sf::Vector2f{16.f, 17.f};
            if (endsWith(packPath, "_damage.png"))
                asset.previewFrame = asset.frames - 1;
            return;
        }
    }
    else if (startsWith(asset.id, "ui/"))
    {
        asset.source = startsWith(asset.id, "ui/kenney-pixel-adventure/") ? "Kenney" : "Beltbound";
        asset.category = "UI";
        const auto parent = asset.id.substr(0, asset.id.rfind('/'));
        const auto variantStart = parent.find('/', parent.find('/') + 1);
        if (variantStart != std::string::npos)
            asset.variant = title(std::string_view(parent).substr(variantStart + 1));
    }
    else
    {
        asset.source = "Beltbound";
        const auto slash = asset.id.find('/');
        asset.category = slash == std::string::npos ? "Images" : title(std::string_view(asset.id).substr(0, slash));
        if (asset.category == "Tilesets")
            asset.category = "Atlases";
    }

    // Individual tiles, reference images and the original art have no pack manifest.
    asset.sheetSize = assetPngSize(asset.path);
    asset.frame = {{0, 0}, asset.sheetSize};
    if (asset.id == engineerId && asset.sheetSize == sf::Vector2i{256, 32})
    {
        asset.name = "Field Engineer";
        asset.rows = {"Down", "Down-left", "Left", "Up-left", "Up", "Up-right", "Right", "Down-right"};
        asset.frame.size = {32, 32};
        asset.anchor = {16.f, 30.f};
        asset.rowsAcross = true;
        asset.placeable = true;
    }
    if (asset.id == oreAtlasId)
        asset.name = "Tier 1 Resource Atlas";
}

void addResources(std::vector<AssetEntry>& assets, const AssetEntry& atlas)
{
    if (atlas.sheetSize != sf::Vector2i{128, 64})
        return;
    constexpr std::string_view names[] = {
        "Iron", "Copper", "Coal", "Stone", "Limestone", "Sand", "Clay", "Quartz"
    };
    constexpr std::string_view keys[] = {
        "iron", "copper", "coal", "stone", "limestone", "sand", "clay", "quartz"
    };
    for (int i = 0; i < 8; ++i)
    {
        AssetEntry asset = atlas;
        asset.id += '#';
        asset.id += keys[i];
        asset.name = names[i];
        asset.category = "Resources";
        asset.variant = "Tier 1";
        asset.frame = {{(i % 4) * 32, (i / 4) * 32}, {32, 32}};
        asset.anchor = {16.f, 22.f};
        asset.placeable = true;
        assets.push_back(std::move(asset));
    }
}

bool skipAssetDirectory(std::string_view filename)
{
    return startsWith(filename, ".") || filename == "archives" || filename == "archive" ||
           filename == "retired" || startsWith(filename, "retired-") || filename == "source-art";
}

bool ordinaryAssetPath(const std::filesystem::path& path)
{
#ifdef _WIN32
    // Some Windows junctions are reported as regular directories by std::filesystem.
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0;
#else
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    return !error && std::filesystem::exists(status) && !std::filesystem::is_symlink(status);
#endif
}

void indexFolder(std::vector<AssetEntry>& assets, const std::filesystem::path& root, const SpriteManifest& manifest)
{
    namespace fs = std::filesystem;
    std::error_code error;
    const auto folderStatus = fs::symlink_status(root, error);
    if (error || !fs::is_directory(folderStatus) || !ordinaryAssetPath(root))
        return;
    fs::recursive_directory_iterator entry(root, fs::directory_options::skip_permission_denied, error);
    const fs::recursive_directory_iterator end;
    for (; entry != end; entry.increment(error))
    {
        if (error)
            break;
        const auto path = entry->path();
        const auto filename = path.filename().generic_u8string();
        const auto status = entry->symlink_status(error);
        if (error)
        {
            error.clear();
            continue;
        }
        if (fs::is_symlink(status) || (fs::is_directory(status) &&
            (skipAssetDirectory(lowercase(filename)) || !ordinaryAssetPath(path))))
            entry.disable_recursion_pending();
        if (!fs::is_regular_file(status) || startsWith(filename, "."))
            continue;
        const auto extension = lowercase(path.extension().generic_u8string());
        const bool isFont = extension == ".ttf" || extension == ".otf";
        if (extension != ".png" && !isFont)
            continue;

        AssetEntry asset;
        asset.id = path.lexically_relative(root).generic_u8string();
        // Older builds may still have the retired top-down artwork under tilesets.
        if (startsWith(lowercase(asset.id), "tilesets/the ground "))
            continue;
        asset.name = title(path.stem().generic_u8string());
        asset.path = path;
        if (isFont)
        {
            asset.kind = AssetKind::Font;
            asset.category = "Fonts";
            asset.source = filename == "Tuffy.ttf" ? "Thatcher Ulrich" : "Beltbound";
            asset.rows = {"Sample"};
        }
        else
        {
            inspectPng(asset, manifest);
            if (asset.sheetSize.x <= 0 || asset.sheetSize.y <= 0)
                continue;
            if (asset.id == oreAtlasId)
                addResources(assets, asset);
        }
        assets.push_back(std::move(asset));
    }
}
}

sf::Vector2i assetPngSize(const std::filesystem::path& path)
{
    const std::array<unsigned char, 16> prefix{0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n',
                                               0, 0, 0, 13, 'I', 'H', 'D', 'R'};
    std::array<unsigned char, 24> bytes;
    std::ifstream file(path, std::ios::binary);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size()) ||
        !std::equal(prefix.begin(), prefix.end(), bytes.begin()))
        return {};

    const auto dimension = [&](std::size_t offset) {
        std::uint32_t value = 0;
        for (std::size_t i = offset; i < offset + 4; ++i)
            value = (value << 8) | bytes[i];
        return value <= 0x7fffffff ? static_cast<int>(value) : 0;
    };
    return {dimension(16), dimension(20)};
}

sf::IntRect AssetEntry::frameRect(int row, int column) const
{
    if (row < 0 || static_cast<std::size_t>(row) >= rows.size() || column < 0 || column >= frames)
        return {};
    const sf::Vector2i offset = rowsAcross ? sf::Vector2i{row * frame.size.x, 0}
                                           : sf::Vector2i{column * frame.size.x, row * frame.size.y};
    return {frame.position + offset, frame.size};
}

sf::Vector2f AssetEntry::anchorForRow(int row) const
{
    if (row >= 0 && static_cast<std::size_t>(row) < rowAnchors.size())
        return rowAnchors[row];
    return anchor;
}

AssetCatalog::AssetCatalog(const std::filesystem::path& assetsFolder)
{
    const auto root = std::filesystem::absolute(assetsFolder).lexically_normal();
    const auto studio = root / "mythryl-studio";
    const auto pack = studio / "tiny-warfare";
    const auto manifestPath = pack / "sprites.json";
    SpriteManifest manifest;
    if (ordinaryAssetPath(root) && ordinaryAssetPath(studio) && ordinaryAssetPath(pack) && ordinaryAssetPath(manifestPath))
        manifest = readManifest(manifestPath);
    assets.reserve(manifest.size());
    indexFolder(assets, root, manifest);
    std::sort(assets.begin(), assets.end(), [](const AssetEntry& left, const AssetEntry& right) {
        return left.id < right.id;
    });
}

const std::vector<AssetEntry>& AssetCatalog::entries() const
{
    return assets;
}

const AssetEntry* AssetCatalog::find(std::string_view id) const
{
    const auto match = std::lower_bound(assets.begin(), assets.end(), id,
        [](const AssetEntry& entry, std::string_view key) { return entry.id < key; });
    if (match == assets.end() || match->id != id)
        return nullptr;
    return &*match;
}

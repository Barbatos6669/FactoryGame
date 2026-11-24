#include "AssetBrowser.hpp"
#include "AssetBrowserLayout.hpp"
#include "AssetCatalog.hpp"
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace
{
std::string fold(std::string text)
{
    for (char& c : text)
    {
        if (c == '/' || c == '_' || c == '-' || c == '\\') c = ' ';
        else c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return text;
}
}

AssetBrowser::AssetBrowser(const sf::Font* font) : font(font) {}
bool AssetBrowser::isOpen() const { return open; }
void AssetBrowser::toggle()
{
    open = font && !open;
    opening = open;
    focusSearch = open;
    replaceQuery = open && !query.isEmpty();
    menuOpen = false;
    detailPage = false;
}
bool AssetBrowser::covers(sf::Vector2u size, sf::Vector2i p) const
{
    return open && p.x >= 0 && p.y >= 0 && static_cast<unsigned>(p.x) < size.x && static_cast<unsigned>(p.y) < size.y;
}

void AssetBrowser::prepare(const AssetCatalog& catalog, const AssetPlacement& active)
{
    if (indexed != &catalog)
    {
        indexed = &catalog;
        categories = {"All categories"};
        searchable.clear();
        for (const auto& e : catalog.entries())
        {
            searchable.push_back(fold(e.name + " " + e.category + " " + e.source + " " + e.variant + " " + e.id));
            if (std::find(categories.begin()+1, categories.end(), e.category) == categories.end())
                categories.push_back(e.category);
        }
        std::sort(categories.begin()+1, categories.end());
        category = 0;
        filter(catalog);
    }
    if (opening)
    {
        opening = false;
        for (int i = 0; i < static_cast<int>(matches.size()); ++i)
        {
            const auto& entry = catalog.entries()[matches[i]];
            if (!active.id.empty() && entry.id == active.id)
            {
                inspect(i,catalog);
                row = std::clamp(active.row,0,std::max(0,static_cast<int>(entry.rows.size())-1));
                frame = std::clamp(active.frame,0,std::max(0,entry.frames-1));
                break;
            }
        }
    }
}

void AssetBrowser::filter(const AssetCatalog& catalog)
{
    const auto utf8 = query.toUtf8();
    std::istringstream input(fold(std::string(utf8.begin(),utf8.end())));
    std::vector<std::string> words;
    for (std::string word; input >> word;) words.push_back(std::move(word));
    matches.clear();
    for (int i=0; i<static_cast<int>(catalog.entries().size()); ++i)
    {
        const auto& e = catalog.entries()[i];
        if ((onlyPlaceable && !e.placeable) || (category > 0 && e.category != categories[category])) continue;
        if (std::all_of(words.begin(), words.end(), [&](const std::string& word) {
            return searchable[i].find(word) != std::string::npos;
        })) matches.push_back(i);
    }
    firstRow = 0;
    inspect(matches.empty() ? -1 : 0,catalog);
    detailPage = false;
}

void AssetBrowser::inspect(int match, const AssetCatalog& catalog)
{
    selected = match;
    row = 0;
    frame = match < 0 ? 0 : catalog.entries()[matches[match]].previewFrame;
}

void AssetBrowser::scroll(int distance, const BrowserLayout& layout)
{
    const int availableRows = (static_cast<int>(matches.size()) + layout.cols - 1) / layout.cols;
    firstRow = std::clamp(firstRow + distance, 0, std::max(0,availableRows - layout.rows));
}
void AssetBrowser::reveal(const BrowserLayout& layout)
{
    if (selected < 0) return;
    const int r = selected / layout.cols;
    if (r < firstRow) firstRow = r;
    if (r >= firstRow + layout.rows) firstRow = r - layout.rows + 1;
    scroll(0,layout);
}

void AssetBrowser::adjustFrame(const AssetCatalog& catalog, int dr, int df)
{
    if (selected < 0) return;
    const auto& e = catalog.entries()[matches[selected]];
    const int nr = std::max(1,static_cast<int>(e.rows.size())), nf = std::max(1,e.frames);
    row = (row + dr + nr) % nr;
    frame = (frame + df + nf) % nf;
}
std::optional<AssetPlacement> AssetBrowser::confirmStamp(const AssetCatalog& catalog)
{
    if (selected < 0)
        return std::nullopt;
    const auto& entry = catalog.entries()[matches[selected]];
    if (!entry.placeable)
        return std::nullopt;
    toggle();
    return AssetPlacement{entry.id, row, frame};
}

std::optional<AssetPlacement> AssetBrowser::handleEvent(const sf::Event& event, sf::Vector2u size,
                                                        const AssetCatalog& catalog, const AssetPlacement& active)
{
    if (!open) return std::nullopt;
    const bool justOpened = opening;
    prepare(catalog,active);
    const BrowserLayout layout(size,detailPage);
    if (layout.scale <= 0.f) return std::nullopt;
    if (justOpened) reveal(layout);
    scroll(0,layout);
    const int shown = std::min(layout.menuRows,static_cast<int>(categories.size()));
    menuStart = std::clamp(menuStart,0,static_cast<int>(categories.size())-shown);
    const auto selectCategory = [&](int index) {
        category = index; menuOpen = false; filter(catalog);
    };
    const auto openMenu = [&] {
        focusSearch = false; menuOpen = true; menuFocus = category;
        menuStart = std::clamp(category-shown/2,0,static_cast<int>(categories.size())-shown);
    };
    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) mouse = move->position;
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        using K = sf::Keyboard::Key;
        const K code = key->code;
        if (code == K::Tab || code == K::Escape) { toggle(); return std::nullopt; }
        if (key->control && code == K::F)
        { detailPage=false; menuOpen=false; focusSearch=true; replaceQuery=true; return std::nullopt; }
        if (key->control && code == K::K) { detailPage=false; openMenu(); return std::nullopt; }
        if (key->control && code == K::P) { onlyPlaceable=!onlyPlaceable; menuOpen=false; filter(catalog); return std::nullopt; }
        if (menuOpen)
        {
            if (code == K::Up || code == K::Down)
            {
                menuFocus = std::clamp(menuFocus+(code==K::Down?1:-1),0,static_cast<int>(categories.size())-1);
                menuStart = std::clamp(menuStart, menuFocus-shown+1, menuFocus);
            }
            else if (code == K::Enter) selectCategory(menuFocus);
            return std::nullopt;
        }
        if (focusSearch && key->control && code == K::A) { replaceQuery=true; return std::nullopt; }
        if (focusSearch && key->control && code == K::V)
        {
            if (replaceQuery) query.clear();
            replaceQuery=false;
            for (auto c : sf::Clipboard::getString())
                if (c >= 32 && c != 127 && query.getSize() < 160) query += sf::String(c);
            filter(catalog);
            return std::nullopt;
        }
        if (focusSearch && code == K::Backspace && !query.isEmpty())
        {
            if (replaceQuery) query.clear(); else query.erase(query.getSize()-1);
            replaceQuery=false; filter(catalog); return std::nullopt;
        }
        if (code == K::Enter)
        {
            if (focusSearch) { focusSearch=false; detailPage=selected>=0; reveal(layout); }
            else if (!layout.split && !detailPage && selected>=0) detailPage=true;
            else return confirmStamp(catalog);
            return std::nullopt;
        }
        if (!focusSearch && code==K::Backspace && detailPage) {detailPage=false; return std::nullopt;}
        if (!focusSearch)
        {
            if (code==K::LBracket) adjustFrame(catalog,-1,0);
            if (code==K::RBracket) adjustFrame(catalog,1,0);
            if (code==K::Comma) adjustFrame(catalog,0,-1);
            if (code==K::Period) adjustFrame(catalog,0,1);
        }
        int next = selected;
        if (code == K::Down) next += layout.cols;
        else if (code == K::Up) next -= layout.cols;
        else if (!focusSearch && code == K::Left) --next;
        else if (!focusSearch && code == K::Right) ++next;
        else if (code == K::PageDown) next += layout.capacity();
        else if (code == K::PageUp) next -= layout.capacity();
        else if (!focusSearch && code == K::Home) next = 0;
        else if (!focusSearch && code == K::End) next = static_cast<int>(matches.size())-1;
        else return std::nullopt;
        if (!matches.empty())
        {
            focusSearch = false;
            inspect(std::clamp(next,0,static_cast<int>(matches.size())-1),catalog);
            reveal(layout);
        }
    }
    if (const auto* typed = event.getIf<sf::Event::TextEntered>())
    {
        if (focusSearch && !menuOpen && typed->unicode >= 32 && typed->unicode != 127 &&
            (replaceQuery || query.getSize()<160))
        {
            if (replaceQuery) query.clear();
            replaceQuery=false; query += sf::String(typed->unicode); filter(catalog);
        }
    }
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        mouse=wheel->position;
        if (wheel->wheel==sf::Mouse::Wheel::Vertical && wheel->delta!=0.f)
        {
            const int steps = static_cast<int>(std::ceil(std::abs(wheel->delta))) * (wheel->delta>0 ? -1 : 1);
            if (menuOpen) menuStart=std::clamp(menuStart+steps,0,static_cast<int>(categories.size())-shown);
            else if (!layout.detail && layout.results.contains(layout.point(mouse))) scroll(steps,layout);
        }
    }
    if (const auto* click = event.getIf<sf::Event::MouseButtonPressed>())
    {
        mouse=click->position;
        if (click->button != sf::Mouse::Button::Left) return std::nullopt;
        const auto p=layout.point(mouse);
        if (layout.close.contains(p)) { toggle(); return std::nullopt; }
        if (menuOpen)
        {
            for (int i=0; i<shown; ++i)
                if (layout.menuItem(i).contains(p)) { selectCategory(menuStart+i); return std::nullopt; }
            menuOpen=false;
            return std::nullopt;
        }
        focusSearch=false;
        replaceQuery=false;
        if (!layout.detail)
        {
            if (layout.search.contains(p))
            {
                focusSearch=true;
                if (layout.clear.contains(p)) {query.clear(); filter(catalog);}
                return std::nullopt;
            }
            if (layout.categories.contains(p)) { openMenu(); return std::nullopt; }
            if (layout.placeable.contains(p)) { onlyPlaceable=!onlyPlaceable; filter(catalog); return std::nullopt; }
            if (layout.previous.contains(p)) { scroll(-layout.rows,layout); return std::nullopt; }
            if (layout.next.contains(p)) { scroll(layout.rows,layout); return std::nullopt; }
            if (matches.empty() && layout.reset.contains(p))
            { query.clear(); category=0; onlyPlaceable=false; filter(catalog); return std::nullopt; }
            for (int i=0; i<layout.capacity(); ++i)
            {
                const int match=firstRow*layout.cols+i;
                if (match>=static_cast<int>(matches.size())) break;
                if (layout.card(i).contains(p)) { inspect(match,catalog); detailPage=!layout.split; return std::nullopt; }
            }
        }
        if ((layout.split || layout.detail) && selected>=0)
        {
            if (layout.detail && layout.back.contains(p)) detailPage=false;
            else if (layout.place.contains(p)) return confirmStamp(catalog);
            else if (layout.rowPrev.contains(p)) adjustFrame(catalog,-1,0);
            else if (layout.rowNext.contains(p)) adjustFrame(catalog,1,0);
            else if (layout.framePrev.contains(p)) adjustFrame(catalog,0,-1);
            else if (layout.frameNext.contains(p)) adjustFrame(catalog,0,1);
        }
    }
    return std::nullopt;
}

#include "AssetBrowser.hpp"
#include "AssetBrowserLayout.hpp"
#include "AssetArt.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

namespace
{
using Rect = sf::FloatRect;
constexpr sf::Color panel{24,31,38}, card{38,48,56}, well{19,26,32}, edge{67,81,89};
constexpr sf::Color hover{53,67,75}, white{240,244,240}, muted{178,192,199}, amber{245,192,114};
constexpr sf::Color selectedFill{68,55,42};

void box(sf::RenderTarget& t, Rect r, sf::Color fill, sf::Color border=sf::Color::Transparent)
{
    sf::RectangleShape shape(r.size); shape.setPosition(r.position); shape.setFillColor(fill);
    shape.setOutlineThickness(-1.f); shape.setOutlineColor(border); t.draw(shape);
}

void text(sf::RenderTarget& t, const sf::Font& font, const sf::String& label, unsigned px, Rect rect,
          sf::Color color, bool center=false, bool bold=false)
{
    if (rect.size.x <= 0.f || label.isEmpty()) return;
    sf::Text ink(font,label,px);
    if (bold) ink.setStyle(sf::Text::Bold);
    if (ink.getLocalBounds().size.x > rect.size.x)
    {
        // Find the largest fitting prefix, even for long asset IDs.
        std::size_t low=0, high=label.getSize();
        while (low<high)
        {
            const std::size_t mid=(low+high+1)/2;
            ink.setString(label.substring(0,mid)+"...");
            if (ink.getLocalBounds().size.x<=rect.size.x) low=mid; else high=mid-1;
        }
        ink.setString(label.substring(0,low)+"...");
    }
    ink.setFillColor(color);
    const Rect bounds=ink.getLocalBounds();
    const float x=rect.position.x+(center?(rect.size.x-bounds.size.x)/2.f:0.f)-bounds.position.x;
    const float y=rect.position.y+(rect.size.y-bounds.size.y)/2.f-bounds.position.y;
    ink.setPosition({std::round(x),std::round(y)});t.draw(ink);
}

Rect inset(Rect r, float x, float y)
{
    return {r.position+sf::Vector2f{x,y},r.size-sf::Vector2f{2*x,2*y}};
}
}

void AssetBrowser::draw(sf::RenderTarget& target, const AssetCatalog& catalog, AssetArt& art,
                        const AssetPlacement& active, bool indexing)
{
    if (!open || !font) return;
    const bool justOpened=opening;
    prepare(catalog,active);
    const BrowserLayout l(target.getSize(),detailPage);
    if (l.scale<=0.f) return;
    if (justOpened) reveal(l);
    scroll(0,l);
    const auto cursor=l.point(mouse);
    const sf::View saved=target.getView();
    target.setView(sf::View(Rect{{0.f,0.f},l.screen}));
    box(target,{{0,0},l.screen},{7,12,18,174});
    box(target,l.panel,panel,edge);
    const auto button=[&](Rect r, const sf::String& title, bool on=false, bool enabled=true) {
        const bool hot=enabled && r.contains(cursor);
        box(target,r,on?amber:hot?hover:enabled?card:well,hot||on?amber:edge);
        text(target,*font,title,14,inset(r,4,0),on?panel:enabled?white:muted,true,on);
    };
    text(target,*font,"Asset library",23,{{l.search.position.x,l.panel.position.y+4.f},{l.panel.size.x-125.f,34.f}},white,false,true);
    if (l.split)
        text(target,*font,"Ctrl+F  Search     Ctrl+K  Categories",13,
             {{l.search.position.x+157.f,l.panel.position.y+8.f},{l.panel.size.x-296.f,28.f}},muted);
    button(l.close,"Esc  Close");
    if (!l.detail)
    {
        box(target,l.search,focusSearch&&replaceQuery&&!query.isEmpty()?selectedFill:well,focusSearch?amber:edge);
        Rect field=inset(l.search,8,0);field.size.x-=24;
        sf::String value=query;
        if (focusSearch) value += "|";
        if (query.isEmpty())
            text(target,*font,focusSearch?"|  Search assets":"Search assets",15,field,muted);
        else text(target,*font,value,15,field,white);
        if (!query.isEmpty()) text(target,*font,"x",15,l.clear,l.clear.contains(cursor)?amber:muted,true);
        button(l.categories,categories[category]+"  v",menuOpen || category!=0);
        button(l.placeable,"Placeable only",onlyPlaceable);

        if (matches.empty())
        {
            const auto mid=l.results.getCenter();
            const char* message = indexing ? "Indexing installed assets..." :
                                  catalog.entries().empty() ? "No installed assets" : "No matching assets";
            text(target,*font,message,19,
                 {{l.results.position.x,mid.y-21.f},{l.results.size.x,24.f}},white,true,true);
            if (!catalog.entries().empty()) button(l.reset,"Reset filters");
        }
        const int start=firstRow*l.cols, count=static_cast<int>(matches.size());
        for(int slot=0; slot<l.capacity() && start+slot<count; ++slot)
        {
            const int index=start+slot;
            const auto& entry=catalog.entries()[matches[index]];
            const Rect b=l.card(slot);
            const bool inspected=index==selected;
            box(target,b,inspected?selectedFill:b.contains(cursor)?hover:card,inspected?amber:edge);
            const Rect preview{{b.position.x+5.f,b.position.y+4.f},{b.size.x-10.f,b.size.y-38.f}};
            box(target,preview,well);
            if (!art.drawPreview(target,entry,inspected?row:0,inspected?frame:entry.previewFrame,inset(preview,3,2)))
                text(target,*font,"Unavailable",12,preview,muted,true);
            const float base=b.position.y+b.size.y-32.f;
            text(target,*font,entry.name,14,{{b.position.x+6,base},{b.size.x-12.f,16}},white,false,true);
            const auto& sub=entry.variant.empty()?entry.category:entry.variant;
            text(target,*font,sub,12,{{b.position.x+6,base+16.f},{b.size.x-12.f,15}},muted);
            if (entry.id==active.id)
                box(target,{b.position,{4.f,b.size.y}},amber);
        }
        button(l.previous,"Prev",false,firstRow>0);
        button(l.next,"Next",false,start+l.capacity()<count);
        const std::string range=count?std::to_string(start+1)+"-"+std::to_string(std::min(count,start+l.capacity()))+" of ":"";
        text(target,*font,range+std::to_string(count),13,l.info,muted,true);
    }

    if (l.split || l.detail)
    {
        const Rect side=l.inspector;box(target,side,well,edge);
        if (selected>=0)
        {
            const auto& entry=catalog.entries()[matches[selected]];
            const float x=side.position.x+10.f, y=side.position.y+6.f, w=side.size.x-20.f;
            const char* kind=!entry.placeable?"REFERENCE ONLY":entry.groundSurface?"TERRAIN STAMP":"OBJECT STAMP";
            text(target,*font,kind,11,{{x,y},{w,16}},entry.placeable?amber:muted,false,true);
            text(target,*font,entry.name,17,{{x,y+18},{w,24}},white,false,true);
            const float available=l.rowPrev.position.y-(y+46.f)-5.f;
            const float metadataHeight=available>=154.f?60.f:available>=92.f?32.f:15.f;
            Rect preview{{x,y+44.f},{w,std::max(1.f,available-metadataHeight)}};
            box(target,preview,panel,edge);
            if (!art.drawPreview(target,entry,row,frame,inset(preview,4,2)))
                text(target,*font,"Preview unavailable",13,preview,muted,true);
            const float metaY=preview.position.y+preview.size.y+2.f;
            if (metadataHeight>=32.f)
                text(target,*font,entry.source+"  /  "+entry.category,12,{{x,metaY},{w,15}},muted);
            const std::string detail=entry.variant.empty()?entry.name:entry.variant;
            text(target,*font,detail,13,{{x,metaY+(metadataHeight>=32.f?16.f:0.f)},{w,15}},white);
            if (metadataHeight>=60.f)
                text(target,*font,entry.id,11,{{x,metaY+34.f},{w,18}},muted);
            const auto selector=[&](Rect prev,Rect next,const std::string& value,bool enabled) {
                button(prev,"<",false,enabled);button(next,">",false,enabled);
                text(target,*font,value,13,{{prev.position.x+prev.size.x+2,prev.position.y},
                     {next.position.x-prev.position.x-prev.size.x-4,prev.size.y}},enabled?white:muted,true);
            };
            const int nrows=static_cast<int>(entry.rows.size());
            const auto rowName=nrows?entry.rows[static_cast<std::size_t>(row)]:"Single row";
            selector(l.rowPrev,l.rowNext,rowName+"   "+std::to_string(row+1)+"/"+std::to_string(std::max(1,nrows)),nrows>1);
            selector(l.framePrev,l.frameNext,"Frame   "+std::to_string(frame+1)+" / "+std::to_string(entry.frames),entry.frames>1);
            if (l.detail) button(l.back,"Back");
            button(l.place,entry.placeable?"Use stamp   Enter":"Reference only",entry.placeable,entry.placeable);
        }
        else
            text(target,*font,"Select an asset to inspect",14,side,muted,true);
    }
    if (menuOpen)
    {
        const int shown=std::min(l.menuRows,static_cast<int>(categories.size()));
        menuStart=std::clamp(menuStart,0,static_cast<int>(categories.size())-shown);
        for (int i=0;i<shown;++i)
        {
            const int index=menuStart+i;const Rect r=l.menuItem(i);
            box(target,r,index==category?selectedFill:r.contains(cursor)||index==menuFocus?hover:card,edge);
            text(target,*font,categories[index],14,inset(r,7,0),index==category?amber:white);
        }
    }
    target.setView(saved);
}

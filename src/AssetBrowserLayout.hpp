#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <algorithm>
#include <cmath>

// Screens use the same logical rectangles for rendering and mouse input.
struct BrowserLayout
{
    using Rect = sf::FloatRect;
    float scale = 1.f;
    sf::Vector2f screen;
    Rect panel, close, search, clear, categories, placeable, results, inspector;
    Rect previous, next, info, reset, back, place, rowPrev, rowNext, framePrev, frameNext;
    int cols = 1, rows = 1, menuRows = 1;
    float cardW = 0, cardH = 0;
    bool split = false, detail = false;

    BrowserLayout(sf::Vector2u size, bool showDetail)
    {
        const float high = size.x >= 2560 && size.y >= 1800 ? 2.f :
                           size.x >= 1920 && size.y >= 1260 ? 1.5f : 1.f;
        scale = std::min({high, size.x / 320.f, size.y / 290.f});
        if (scale <= 0.f) return;
        screen = sf::Vector2f(size) / scale;
        const float w = std::min(1120.f, std::floor(screen.x) - 12.f);
        const float h = std::min(704.f, std::floor(screen.y) - 12.f);
        const float x = std::floor((screen.x - w) / 2.f), y = std::floor((screen.y - h) / 2.f);
        panel = {{x,y},{w,h}};
        split = w >= 718.f && h >= 420.f;
        detail = !split && showDetail;
        const float pad = w < 520.f ? 9.f : 16.f;
        const float left = x + pad, right = x + w - pad, bottom = y + h - pad;
        const float inner = right - left;
        close = {{right - 82.f, y + 9.f}, {82.f, 26.f}};
        const float filterY = y + 44.f;
        const bool narrow = w < 570.f;
        search = {{left,filterY},{narrow ? inner : inner - 304.f,30.f}};
        clear = {{search.position.x + search.size.x - 28.f,filterY},{28.f,30.f}};
        const float cy = narrow ? filterY + 36.f : filterY;
        categories = {{narrow ? left : right - 297.f,cy},{narrow ? inner - 109.f : 177.f,30.f}};
        placeable = {{categories.position.x + categories.size.x + 6.f,cy},
                     {right - (categories.position.x + categories.size.x + 6.f),30.f}};
        const float contentY = cy + 40.f;
        const float detailW = split ? std::min(284.f, std::floor(inner * 0.32f)) : inner;
        inspector = {{right - detailW, detail ? y + 44.f : contentY}, {detailW,0.f}};
        inspector.size.y = bottom - inspector.position.y;
        const float resultsW = split ? inner - detailW - 12.f : inner;
        results = {{left,contentY},{resultsW,bottom - contentY - 37.f}};
        cols = std::max(2, static_cast<int>((resultsW+6.f)/132.f));
        rows = std::max(1,static_cast<int>((results.size.y+6.f)/91.f));
        cardW = std::floor((resultsW - 6.f*(cols-1))/cols);
        cardH = std::floor((results.size.y - 6.f*(rows-1))/rows);
        previous = {{left,bottom - 29.f},{62.f,29.f}};
        next = {{left + resultsW - 62.f,bottom - 29.f},{62.f,29.f}};
        info = {{left + 65.f,bottom - 29.f},{resultsW-130.f,29.f}};
        reset = {results.getCenter()+sf::Vector2f{-54.f,8.f},{108.f,27.f}};
        back = {{inspector.position.x + 8.f,bottom-32.f},{60.f,32.f}};
        const float ix = inspector.position.x + 10.f, iw = inspector.size.x - 20.f;
        place = {{detail ? back.position.x+69.f : ix,bottom-32.f},{detail ? iw-67.f : iw,32.f}};
        rowPrev = {{ix,bottom-104.f},{29.f,27.f}};
        rowNext = {{ix+iw-29.f,bottom-104.f},{29.f,27.f}};
        framePrev = {{ix,bottom-70.f},{29.f,27.f}};
        frameNext = {{ix+iw-29.f,bottom-70.f},{29.f,27.f}};
        menuRows = std::max(1,static_cast<int>((bottom - (cy+33.f))/26.f));
    }
    Rect card(int slot) const
    {
        return {{results.position.x + (slot%cols)*(cardW+6.f),
                 results.position.y + (slot/cols)*(cardH+6.f)}, {cardW,cardH}};
    }
    Rect menuItem(int slot) const
    {
        return {{categories.position.x,categories.position.y+33.f+26.f*slot},{categories.size.x,26.f}};
    }
    sf::Vector2f point(sf::Vector2i p) const { return (sf::Vector2f(p) + sf::Vector2f{0.5f,0.5f}) / scale; }
    int capacity() const { return cols*rows; }
};

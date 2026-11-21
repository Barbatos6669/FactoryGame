#pragma once

#include <SFML/Graphics/RenderTarget.hpp>

/**
 * @brief A square grid beginning at world position (0, 0).
 * Cell dimensions use world units, matching pixels in the current unscaled view.
 */
class Grid
{
public:
    int size = 8;      ///< Number of cells along each side.
    int cellSize = 32; ///< Side length of one cell in world units.

    /**
     * @brief Get the extent of one side of the square grid.
     * @return Side length in world units (pixels in the current unscaled view).
     */
    int getPixelSize() const;

    /**
     * @brief Assign the grid size directly, without validation.
     * @param newSize Number of cells along each side.
     */
    void setGridSize(int newSize);

    /**
     * @brief Check whether a world position is inside the grid.
     * @param worldPosition Position in world coordinates.
     * @return True for positive grid and cell sizes with both coordinates in
     *         the half-open interval [0, getPixelSize()).
     */
    bool contains(sf::Vector2f worldPosition) const;

    /**
     * @brief Convert an inside-grid world position to a cell index.
     * @param worldPosition Position in world coordinates.
     * @pre contains(worldPosition) returns true.
     * @return Zero-based indices: x is the column and y is the row.
     */
    sf::Vector2i worldToCell(sf::Vector2f worldPosition) const;

    /**
     * @brief Draw white lines one world unit thick and a green grid outline.
     * Uses the active view without changing it; call after drawing a hover fill.
     * @param target Render target for the grid.
     */
    void draw(sf::RenderTarget& target) const;
};

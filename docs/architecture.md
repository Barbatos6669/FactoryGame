# Current architecture

Beltbound is a small C++17/SFML 3 learning project. The map stores 100 rows and 100 columns of logical cells, each 32 map units on a side. At 1× zoom each cell appears as a 32-by-16-pixel diamond. Painted cells live in a vector of `Tile` records.

## File roles

- `src/main.cpp` sets up the window, routes events, updates the camera and runs the frame loop.
- `src/MapEditor.cpp` owns brush and tool selection and groups map changes into undoable edits. Follow `selectBrush()` and `selectTool()` to see how Raise remembers the last terrain brush. `src/EditHistory.cpp` records and restores the affected cells.
- `include/Grid.hpp` declares `Grid`, its `size` and `cellSize` fields, and methods for dimensions, coordinate lookup, and drawing. `#pragma once` prevents repeated inclusion.
- `src/Grid.cpp` handles map bounds, logical positions, and drawing grid lines and diamond outlines.
- `include/Tile.hpp` defines a cell's bottom-to-top list of `TerrainLayer` records. `src/Tile.cpp` contains the six startup examples, editing operations, and save/load.
- `src/TerrainScene.cpp` draws columns back to front, picks a visible column for mouse input, and looks up the surface height for the engineer.
- `include/Projection.hpp` converts between logical map coordinates and the isometric view. `src/IsometricArt.cpp` supplies the ground, rocks, and wildflowers for the map and brush previews. It uses Tiny Warfare temperate terrain and rocks when installed and built-in shapes otherwise.
- `include/Engineer.hpp` and `src/Engineer.cpp` own the woodland engineer's position, facing, animation clock, and idle/walk textures. Each tick uses the change in his feet position to choose his facing and animation.
- `include/Debug.hpp` and `src/Debug.cpp` provide optional console logging and the on-screen debug overlay.
- `include/EditorUI.hpp` and `src/EditorUI.cpp` lay out the screen-space palette, draw the controls and feedback, and identify the control under a pointer. The editor shares the diagnostics font and tile textures.
- `AssetCatalog.cpp` reads the optional Tiny Warfare manifest and scans installed visual assets. Each entry has an ID relative to the assets folder, a category, frame geometry and anchor. `AssetBrowser.cpp` handles search, navigation and stamp selection. `AssetBrowserDraw.cpp` draws the library.
- `AssetArt.cpp` indexes on a background worker on first use and caches the individual frames drawn by the library and map. Each cached frame has a texture for drawing and opacity data for picking. The cache can hold up to 512 frames within 48 MiB.
- `assets/fonts/` contains the unmodified Tuffy 1.1 font, its public-domain notice, and source attribution. The runtime font path is `assets/fonts/Tuffy.ttf` relative to the executable's folder, resolved from `argv[0]` in `main()`.
- `build.ps1` compiles the `.cpp` files listed in the Makefile, adds `include/` and SFML's headers to the include search path, links with SFML, and copies runtime DLLs and `assets/` into `build/`.
- `.vscode/` connects Ctrl+Shift+B and F5 to the build script and configures C++ IntelliSense and GDB.
- `docs/CHANGELOG.md` records development checkpoints. `build/` contains generated output, and `vendor/` contains the downloaded SFML package; both are ignored by Git.

## Startup and each frame

`main()` owns the window, grid, diagnostics, isometric artwork, and woodland engineer. `runGame()` owns the view, clock, palette, and `MapEditor`. The editor holds the cells, selected tool and brush, last terrain brush, save path, and zoom. The six examples start near the center with the engineer on plain grass at (50,50).

The grid starts at world position (0,0). The view starts at 1×, centered on the engineer's feet, or on the map midpoint in editor-only mode. The vertical scroll wheel adds 0.125× per notch, including fractional input, within the range 1× to 1.5×. On a wheel event or resize, the world view keeps its center and sets its size to the window dimensions divided by the current zoom. The updated view applies immediately so the next click uses the new scale.

W/A/S/D walk the engineer at 96 logical map units per second. The direction keys move toward their screen directions. Opposite keys cancel, and diagonal movement is normalized. The feet stay within the map. If one axis is blocked at the edge, the engineer can slide along it. At rest he keeps the direction of his last actual step. A movement tick uses up to 0.05 seconds, and frames with a focus change or without focus suppress movement.

Every walking update moves the engineer first, then sets the view center to his new feet position before mapping the mouse and drawing. Release the movement keys to pan with the arrows at 400 pixels per second. Panning keeps the camera center within the map. The next step centers the view on the engineer again. On installations without the engineer sheets, WASD and the arrows pan the editor.

`runGame()` polls for a finished catalog, handles events, updates the engineer and camera, picks the visible cell under the mouse, updates diagnostics, and draws. While the library is open, its controls receive mouse and keyboard input and walking and camera panning pause. A catalog is requested when the player opens the library or loads a map with asset placements.

`handleEvents()` checks the palette and diagnostics before passing a left map click to `MapEditor::pressCell()`. Follow the drag through `dragTo()`. On release, the window sends the final pointer position and calls `finishEdit()`. A right click calls `clearDecoration()`, which finishes its own edit. Ctrl+Z calls `undoEdit()`. Buttons and their keyboard shortcuts share `applyEditorCommand()`. Both panels use screen pixels.

## Isometric view

Click and hover use the same lookup. When stacks overlap, the foremost painted column under the pointer wins. The exposed side faces also count. Over empty space, the inverse projection gives the cell on the base plane.

Holding the left button with a terrain brush or Erase starts a stroke. Movement and release trace the screen pixels between reported mouse positions. A temporary index groups nearby columns for these lookups and adds new cells as they are painted. The lookup uses the same sprite opacity and depth rules as a click. Each visible cell is edited once per stroke.

An Erase click immediately removes the exposed layer, including the base if it is the last block. If the pointer moves, the stroke picks against the columns as they looked at the press. A return pass over a freshly exposed tile therefore leaves it alone until the next press. Erased base cells have no save row. An empty save represents an empty map.

`EditHistory` remembers a cell's original layers the first time an edit touches it. An Erase gesture also remembers the starting order of the coordinates so undo can return removed columns to their original save rows. Releasing or interrupting a stroke finishes one history entry. Other map edits finish after each click. Ctrl+Z restores the previous edit in memory. F2 writes the current map, and a successful F4 clears its undo history.

History keeps up to 32 edits with a target of 32 MiB. It drops older edits first and keeps the newest gesture whole, even if that gesture alone exceeds the target.

Columns draw back to front. Within a column, each block draws eight pixels higher than the last at native size. Only the exposed block draws its decoration. The engineer's feet and following camera use his cell's surface height. The grid and yellow hover outline follow each top face.

Save files use logical cell coordinates. A one-block cell uses the original `50,50 dirt flower` form. A taller cell uses pairs from bottom to top, such as `50,50 stack dirt flower grass none`. Lowering that column exposes the original flowers. An old `50,50` row loads as a single grass block.

A layer can also store a catalog ground surface and a catalog decoration. An `assets` row records their relative IDs and zero-based rows and frames. Only the exposed decoration is drawn. Stamps use their source anchor and native scale at 1×. A building is anchored to the clicked cell, and its sprite may extend across neighboring cells. A missing optional object is marked on the map, and missing terrain uses the block's base material. Loading and saving retain the original references.

To follow the code, start at `main()` for configuration, then read `runGame()` for one frame's order. Put a breakpoint in `runGame()` and step into one named helper at a time; `Grid.cpp` contains the coordinate math and grid drawing, while `Debug.cpp` contains the diagnostics.

## Debug behavior

The grid and debug overlay start visible. F3 toggles both once per keypress. FPS is averaged over roughly 0.5 seconds. The overlay shows mouse pixels, the projected point on the base plane, the actual hovered cell, grid dimensions, and map size.

`Debug::draw()` temporarily uses a pixel-based screen view to keep the panel at the top-right, then restores the previous game view. If the font failed to load, drawing the overlay is skipped and the game continues. `loadFont()` reports the failure once through `log()`.

`setConsoleEnabled(false)` silences `Debug::log()` without hiding the overlay. Startup messages, visibility changes, and valid clicked-cell indices use `log()`.

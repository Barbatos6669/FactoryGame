# Current architecture

Beltbound is a small C++17/SFML 3 learning project. The grid is square: one size controls both axes. `main()` calls `grid.setGridSize(100)`; its 100 cells per side and 32 pixels per cell produce a 3200 x 3200 pixel grid. The class default remains 8 cells per side. Painted cells live in a vector of `Tile` records.

## File roles

- `src/main.cpp` keeps startup configuration in `main()`, with named helpers for window setup, events, camera movement, hover drawing, frame drawing, and running the game loop.
- `include/Grid.hpp` declares `Grid`, its `size` and `cellSize` fields, and methods for dimensions, coordinate lookup, and drawing. `#pragma once` prevents repeated inclusion.
- `src/Grid.cpp` defines those methods. `contains()` checks world-coordinate bounds; after it succeeds, `worldToCell()` converts the world coordinates to cell indices. Hover and debug use this shared lookup. `draw()` renders the green border and grid lines. The header tells other files what the class provides; the source file contains the methods' implementation.
- `include/Tile.hpp` defines the two layers of a cell. `src/Tile.cpp` contains the six startup examples, painting, and save/load.
- `include/Debug.hpp` and `src/Debug.cpp` provide optional console logging and the on-screen debug overlay.
- `assets/fonts/` contains the unmodified Tuffy 1.1 font, its public-domain notice, and source attribution. The runtime font path is `assets/fonts/Tuffy.ttf` relative to the executable's folder, resolved from `argv[0]` in `main()`.
- `build.ps1` compiles the `.cpp` files listed in the Makefile, adds `include/` and SFML's headers to the include search path, links with SFML, and copies runtime DLLs and `assets/` into `build/`.
- `.vscode/` connects Ctrl+Shift+B and F5 to the build script and configures C++ IntelliSense and GDB.
- `docs/CHANGELOG.md` records development checkpoints. `build/` contains generated output, and `vendor/` contains the downloaded SFML package; both are ignored by Git.

## Startup and each frame

`main()` owns the window, configured `Grid`, `Debug`, tile textures, and hover outline. `runGame()` owns the persistent view, frame clock, selected brush, and painted tiles. `makeSampleTiles()` starts it off with six examples near the center, including the same flowers on grass and dirt.

The grid stays at the world origin. A persistent game view starts centered on the grid's midpoint and moves with WASD. Resizing changes that view's size to the window's pixel dimensions while preserving its bounded camera center, so cells stay square and one game unit stays one pixel.

Camera movement reads simultaneous held keys while the window has focus. W/A/S/D move up/left/down/right at `CAMERA_SPEED` (400 world units per second); opposite keys cancel and diagonal movement is normalized. Movement uses frame time capped at 0.05 seconds to limit jumps after a stall, and skips frames containing focus changes.

After movement, each camera-center coordinate is clamped to the inclusive range `[0, grid.getPixelSize()]`, currently `[0, 3200]`. Clamping each axis allows sliding along an edge and immediate reversal. The viewport may extend beyond the map; resizing preserves the bounded center.

After measuring frame time, `runGame()` follows this order, stopping after event handling if the window closed:

```cpp
const bool focusChanged = handleEvents(window, view, debug, grid, tiles, brush, savePath);
updateCamera(window, view, grid, deltaSeconds, focusChanged);
const auto mousePixels = sf::Mouse::getPosition(window);
const auto mouseWorld = window.mapPixelToCoords(mousePixels);
debug.update(deltaSeconds, mousePixels, mouseWorld, grid);
drawFrame(window, grid, debug, mouseWorld, art, outline, tiles);
```

`handleEvents()` maps each click through the active view. Inside the grid, `paintTile()` finds the cell or adds it once, then changes its terrain or its object according to the selected brush. `clearTileObject()` removes the object on a right-click. `drawTiles()` draws each terrain sprite first and its object second. `drawFrame()` then draws the optional grid, the hover outline, and the debug overlay.

`Grid::draw()` keeps the green `RectangleShape` frame and uses full-length horizontal and vertical `RectangleShape` lines. One loop runs `size` times, drawing two lines per index. This gives `2 * size + 1` top-level `target.draw()` calls per grid frame: 201 at the current size of 100. Drawing work grows linearly with cells per side.

To follow the code, start at `main()` for configuration, then read `runGame()` for one frame's order. Put a breakpoint in `runGame()` and step into one named helper at a time; `Grid.cpp` contains the coordinate math and grid drawing, while `Debug.cpp` contains the diagnostics.

## Debug behavior

The grid and debug overlay start visible. F3 toggles both once per keypress. FPS is averaged over roughly 0.5 seconds. Other values show mouse pixels, mouse world coordinates, the zero-based hovered cell or `outside-grid`, grid dimensions, cell size, and map size. Hover highlighting and the diagnostic cell lookup use the same world coordinates.

`Debug::draw()` temporarily uses a pixel-based screen view to keep the panel at the top-right, then restores the previous game view. If the font failed to load, drawing the overlay is skipped and the game continues. `loadFont()` reports the failure once through `log()`.

`setConsoleEnabled(false)` silences `Debug::log()` without hiding the overlay. Startup messages, visibility changes, left-click map coordinates, and valid clicked-cell indices use `log()`.

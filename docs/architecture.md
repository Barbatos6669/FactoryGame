# Current architecture

Beltbound is a small C++17/SFML 3 learning project. The grid is square: one size controls both axes. `main()` calls `grid.setGridSize(100)`; its 100 cells per side and 32 pixels per cell produce a 3200 x 3200 pixel grid. The class default remains 8 cells per side. Tile occupancy and placement are not implemented yet.

## File roles

- `src/main.cpp` keeps startup configuration in `main()`, with named helpers for window setup, events, camera movement, hover drawing, frame drawing, and running the game loop.
- `include/Grid.hpp` declares `Grid`, its `size` and `cellSize` fields, and methods for dimensions, coordinate lookup, and drawing. `#pragma once` prevents repeated inclusion.
- `src/Grid.cpp` defines those methods. `contains()` checks world-coordinate bounds; after it succeeds, `worldToCell()` converts the world coordinates to cell indices. Hover and debug use this shared lookup. `draw()` renders the green border and grid lines. The header tells other files what the class provides; the source file contains the methods' implementation.
- `include/Debug.hpp` and `src/Debug.cpp` provide optional console logging and the on-screen debug overlay.
- `assets/fonts/` contains the unmodified Tuffy 1.1 font, its public-domain notice, and source attribution. The runtime font path is `assets/fonts/Tuffy.ttf` relative to the executable's folder, resolved from `argv[0]` in `main()`.
- `build.ps1` compiles the three `.cpp` files, adds `include/` and SFML's headers to the include search path, links with SFML, and copies runtime DLLs and `assets/` into `build/`.
- `.vscode/` connects Ctrl+Shift+B and F5 to the build script and configures C++ IntelliSense and GDB.
- `docs/CHANGELOG.md` records development checkpoints. `build/` contains generated output, and `vendor/` contains the downloaded SFML package; both are ignored by Git.

## Startup and each frame

`main()` owns the window, configured `Grid`, and `Debug`. It calls `setupWindow()`, loads the bundled font, logs startup, and passes these objects to `runGame()`. `runGame()` owns the persistent view and frame clock for the duration of the loop.

The grid stays at the world origin. A persistent game view starts centered on the grid's midpoint and moves with WASD. Resizing changes that view's size to the window's pixel dimensions while preserving its bounded camera center, so cells stay square and one game unit stays one pixel.

Camera movement reads simultaneous held keys while the window has focus. W/A/S/D move up/left/down/right at `CAMERA_SPEED` (400 world units per second); opposite keys cancel and diagonal movement is normalized. Movement uses frame time capped at 0.05 seconds to limit jumps after a stall, and skips frames containing focus changes.

After movement, each camera-center coordinate is clamped to the inclusive range `[0, grid.getPixelSize()]`, currently `[0, 3200]`. Clamping each axis allows sliding along an edge and immediate reversal. The viewport may extend beyond the map; resizing preserves the bounded center.

After measuring frame time, `runGame()` follows this order, stopping after event handling if the window closed:

```cpp
const bool focusChanged = handleEvents(window, view, debug);
updateCamera(window, view, grid, deltaSeconds, focusChanged);
const auto mousePixels = sf::Mouse::getPosition(window);
const auto mouseWorld = window.mapPixelToCoords(mousePixels);
debug.update(deltaSeconds, mousePixels, mouseWorld, grid);
drawFrame(window, grid, debug, mouseWorld);
```

`handleEvents()` handles close, focus, F3, and resize. `updateCamera()` applies the focused WASD movement and bounds described above. `drawFrame()` clears the window, calls `drawHover()`, draws the grid and debug overlay, then presents the frame. `drawHover()` uses `Grid`'s shared lookup to place the blue highlight.

`Grid::draw()` keeps the green `RectangleShape` frame and uses full-length horizontal and vertical `RectangleShape` lines. One loop runs `size` times, drawing two lines per index. This gives `2 * size + 1` top-level `target.draw()` calls per grid frame: 201 at the current size of 100. Drawing work grows linearly with cells per side.

To follow the code, start at `main()` for configuration, then read `runGame()` for one frame's order. Put a breakpoint in `runGame()` and step into one named helper at a time; `Grid.cpp` contains the coordinate math and grid drawing, while `Debug.cpp` contains the diagnostics.

## Debug behavior

The overlay starts visible, and F3 toggles it once per keypress. FPS is averaged over roughly 0.5 seconds. Other values show mouse pixels, mouse world coordinates, the zero-based hovered cell or `outside-grid`, grid dimensions, cell size, and map size. Hover highlighting and the diagnostic cell lookup use the same world coordinates.

`Debug::draw()` temporarily uses a pixel-based screen view to keep the panel at the top-right, then restores the previous game view. If the font failed to load, drawing the overlay is skipped and the game continues. `loadFont()` reports the failure once through `log()`.

`setConsoleEnabled(false)` silences `Debug::log()` without hiding the overlay. Startup and overlay-toggle messages use `log()`; there is no mouse-position console output inside the frame loops. To hide the overlay, press F3 or call `toggleOverlay()`.

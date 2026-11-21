# Beltbound development log

Record meaningful changes, validation, and the next milestone here. Newest entries come first. Entries describe development checkpoints, not necessarily releases.

## 2026-11-21 - Documented class APIs and frame helpers

- Added concise Doxygen comments to Grid and Debug header declarations and the ordinary functions in `main.cpp`, covering units, coordinate spaces, lookup preconditions, focus/timing rules, ownership, and drawing order. Preserved the original author and copyright attribution.
- Documented half-open cell containment separately from inclusive camera-center bounds, console/overlay independence, font-loading behavior, and view restoration. Added one implementation note about saving the view by value.
- Validation: compiler-preprocessed output for all three translation units was byte-identical before and after the comment edits. The normal build passed without warnings; camera, rendering, and UI suites were not repeated.

## 2026-11-21 - Refactored the frame flow into ordinary functions

- `main()` now owns the window, configured Grid, and Debug object. Preserved the user's latest size of 100 cells per side. `runGame()` owns the persistent world view and frame clock, and calls named event, camera, and frame helpers in order.
- Extracted `handleEvents()`, `updateCamera()`, `drawHover()`, and `drawFrame()`. Removed the unused Tile definition and commented-out array. Grid's shared `contains()` and `worldToCell()` helpers now serve hover drawing and Debug without duplicated coordinate math.
- Kept existing Grid/Debug APIs, the one-loop RectangleShape renderer, camera/focus/bounds behavior, F3, executable-relative assets, and the existing build workflow. Removed redundant shape defaults and updated the reading guide and architecture notes.
- Validation: the normal build passed without warnings; 19 controlled movement frames and 43 bounds frames (126 bounds assertions) passed camera/resize/focus/hover checks. Six renderer comparisons remained pixel-identical, with 201 draw calls at size 100 and 2,001 at size 1000. Existing Debug checks passed logging, timing, view restoration, toggling, and missing-font fallback. No new live held-key test was performed.

## 2026-11-21 - Drew each grid line once

- Preserved the user's `grid.setGridSize(1000)` and changed only grid rendering: each horizontal rectangle spans the full map width and each vertical rectangle spans the full map height. One loop draws each line once; the green frame remains, and the redundant final repeated draws are removed.
- Top-level grid draw calls are now `2 * size + 1`, rather than `2 * size * size + 3`: 2,001 instead of 2,000,003 at size 1000. The renderer still uses familiar `RectangleShape` objects.
- Validation: the normal build passed without warnings, `src/main.cpp` stayed byte-for-byte unchanged, and six small-grid image comparisons were pixel-identical to the previous renderer, covering the border, hover layering, edges, fractional view positions, and resize. Bounded instrumentation measured 201 calls at size 100 and 2,001 at size 1000; the old million-cell renderer was not run at that size.

## 2026-11-21 - Bounded the camera center to the grid

- Clamped each view-center coordinate after movement to the inclusive range `[0, mapSize]`, before applying the view and mapping the mouse. The user's current 100 x 100 grid at 32 pixels per cell gives bounds `[0, 3200]` on both axes.
- Outward input stops at an edge, diagonal input can slide along it, and reversing moves inward immediately. The viewport can extend beyond the map; resize, focus protection, movement speed, and hover conversion are unchanged.
- Updated the relevant controls and architecture descriptions. Validation: compilation passed without warnings to `build/FactoryGame-camera-bounds-check.exe` while the user's game remained open. Forty-three controlled production-loop frames passed edge/corner overshoot, repeated outward input, inward reversal, sliding, hover/debug alignment, resize, and oversized-viewport checks.

## 2026-11-21 - Added WASD camera panning

- Added real-time W/A/S/D movement to the existing view at `CAMERA_SPEED = 400.f` world units per second in `src/main.cpp`. Opposite keys cancel, and nonzero diagonal input is normalized.
- Camera input requires window focus. Focus-change frames skip movement, and movement time is capped at 50 ms after stalls. Resize preserves the panned center; the updated view is applied before hover and Debug mouse conversion.
- Updated controls and architecture notes. Validation: the project compiled without warnings to `build/FactoryGame-wasd-check.exe`, preserving the running game. Nineteen controlled-input frames of the production loop passed direction, time-scaling, diagonal, focus, stall, release, resize, and hover-alignment checks.
- Live verification was limited to startup and hover alignment: the desktop tool's discrete taps were too brief for real-time key polling, so sustained and simultaneous physical-key movement were not verified live. Only the separate test window was closed.

## 2026-11-21 - Centered the view on the grid

- Kept the grid at world origin and centered one persistent view on its midpoint. Positive-size resize events update only the view's size, preserving its center and 1:1 pixel scale.
- Kept active-view mouse conversion and the screen-space Debug overlay unchanged; updated the short architecture and README descriptions.
- Validation: compilation passed without warnings to `build/FactoryGame-centered-check.exe` using the existing project flags, leaving the running user game open. Narrow offscreen checks at 1920 x 1080, 800 x 600, and 801 x 601 confirmed centering, pixel scale, and hovered-cell alignment.

## 2026-11-20 - Moved grid rendering into Grid

- Declared `draw(sf::RenderTarget&) const` in `include/Grid.hpp` and moved the existing green border and grid-line rendering into `src/Grid.cpp`, using the grid's own square size and cell size.
- The frame loop draws the hover highlight, calls `grid.draw(window)`, then draws Debug and displays the frame. Events, resizing, mouse conversion, clearing, and hover calculations remain in `main.cpp`.
- Updated the architecture notes and README file roles. Validation: the normal build passed without warnings; a narrow offscreen rendering check confirmed blue cell interiors, white lines above the highlight, the green border, changed grid dimensions, and an unchanged render view.

## 2026-11-20 - Added console and on-screen Debug diagnostics

- Added `include/Debug.hpp` and `src/Debug.cpp` with console enable/disable, message logging, overlay toggling, frame-data updates, and drawing.
- F3 toggles the initially visible overlay; repeated key events are disabled. The panel shows averaged FPS, mouse pixels and world coordinates, the hovered cell or outside-grid status, and current grid/cell/map dimensions.
- Replaced the nested-loop mouse console spam with startup and overlay-toggle messages. Console logging and overlay visibility are independent.
- Preserved the user's 10 x 10 grid, `setGridSize()`, and resize handler. Hover and diagnostics share mouse coordinates mapped through the active view. Overlay drawing uses the current target size and restores the previous view.
- Bundled the existing SFML example Tuffy font with its public-domain notice and attribution. The build copies assets beside the executable; a missing font disables the text overlay without preventing play.
- Validation: the normal build passed without warnings. A temporary behavior check verified console controls, toggling, FPS and hover data, screen placement, full view restoration, and missing-font fallback. A separate live test window verified readable diagnostics, matching hover-cell data before and after resizing, outside-grid reporting, and F3 hide/show. Only the test window was closed; the user's game remained open. Held-key repeat was not exercised live.

## 2026-11-20 - Separated Grid declaration and implementation

- Kept the `Grid` class and its public field defaults in `include/Grid.hpp`, with a declaration for `getPixelSize()`.
- Moved the method body into `src/Grid.cpp`; `build.ps1` now compiles and links both application source files.
- Updated the README and architecture notes to explain the header/source split. Drawing and hover code in `src/main.cpp` were left unchanged.
- Validation: the normal build passed without warnings, and GDB resolved `Grid::getPixelSize()` to `src/Grid.cpp`.

## 2026-11-20 - Organized source, headers, and documentation

- Moved application code to `src/main.cpp`, the unchanged grid header to `include/Grid.hpp`, and this log to `docs/CHANGELOG.md`.
- Updated the build script and IntelliSense include path; the existing root build task and `build/FactoryGame.exe` debug target remain valid.
- Added `docs/architecture.md` and updated the README with the actual file roles, folder tree, and current square-grid hover behavior.
- Validation: `build.ps1` rebuilt the normal executable without warnings, GDB resolved `main()` to `src/main.cpp`, README links resolved, and generated output and SFML remained ignored. Source and header contents were preserved byte for byte during the move.

## 2026-11-20 - Started the square Grid class

- Added a small `Grid` class with cells per side, pixels per cell, and `getPixelSize()`.
- Moved the class into `Grid.hpp` with `#pragma once`; `main.cpp` includes the header, and `getPixelSize()` remains defined inside the class.
- Created one grid before the frame loop and connected its dimensions to the existing drawing and hover calculations: 8 x 8 cells, 32 pixels each, 256 pixels per side.
- Validation: compiled without warnings using the project build flags to `build/FactoryGame-grid-check.exe`, since the normal executable was running. Live hover behavior was not retested in this step.
- Header-move validation: `build.ps1` successfully rebuilt `build/FactoryGame.exe` without warnings.
- Next class step: move drawing or hovered-cell lookup into `Grid` when ready.

## 2026-11-20 - Restored build setup

- Restored the missing local WinLibs UCRT GCC 14.2.0 toolchain and SFML 3.1.0 package, using the matching downloads linked by SFML and verifying their checksums.
- Added clear build errors for a missing compiler or SFML headers.
- The build script now copies the three required MinGW runtime DLLs beside the executable, allowing terminal launches without changing PATH.
- Removed the generated active-file build task, which lacked SFML include and linker settings. Use `Build FactoryGame` or F5 with `Debug Beltbound`.
- Validation: the project builds successfully; a GDB startup check created the SFML window and reached the game loop, then stopped the test process. The unused `grid` warning remains. The next gameplay milestone is still drawing and clicking occupied tiles.

## 2026-11-20 - Window, grid, and tile data checkpoint

### Added

- A window titled Beltbound, using constants for its initial 1920 x 1080 size.
- A 60 FPS limit and SFML 3 close-event handling.
- An 8 x 8 visual grid with 32-pixel cells and a green outer frame. The grid occupies 256 x 256 game units.
- A `Tile` structure with an `occupied` flag, initially false.
- A persistent `Tile grid[8][8]` array, created before the frame loop so tile changes can survive between frames.
- A VS Code GDB configuration named `Debug Beltbound`; F5 builds the project before starting the debugger.

### Fixed

- Window setup and the game loop share the same window by reference.
- Window creation and event polling use SFML 3 syntax.
- Grid-frame dimensions are explicitly converted to floats, using both map width and map height.
- The grid frame has a transparent fill so it does not hide the grid lines.

### Validation and current limits

- The current source compiled and linked successfully with GCC 14.2.0 and SFML 3.1.0.
- Validation used a temporary executable because Windows prevented overwriting the normal executable; the temporary executable was removed afterward.
- One compiler warning remains: `grid` is declared but not used yet.
- Tile occupancy does not affect rendering yet. Mouse interaction and save files are not implemented.

### Next milestone

Draw a tile based on its stored `occupied` flag, then allow a mouse click to change that flag.

## 2026-11-18 - Project baseline

- Established the C++17 project with SFML 3.1.0 and a MinGW build script.
- Added VS Code build settings and automatic saving.
- Created the private FactoryGame GitHub repository and committed the initial source.
- Kept build output, downloaded dependencies, and setup backups outside version control. A fresh clone needs the matching compiler and SFML package installed locally.

# Beltbound development log

Record meaningful changes, validation, and the next milestone here. Newest entries come first. Entries describe development checkpoints, not necessarily releases.

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

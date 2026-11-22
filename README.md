# FactoryGame - SFML

A top-down factory game learning project written in C++ with SFML 3.

Working title: Beltbound. See [the development log](docs/CHANGELOG.md) for checkpoints and the next milestone, and [the architecture notes](docs/architecture.md) for how the current code fits together.

## Start
Open this folder in VS Code. Application code is in `src/main.cpp`. The grid class is split between `include/Grid.hpp` and `src/Grid.cpp`.

- Press **Ctrl+Shift+B** for an incremental build, then run `.\build\FactoryGame.exe` in the terminal.
- Press **F5** with **Debug Beltbound** to build incrementally and start the debugger.
- For a full rebuild, open the Command Palette, choose **Tasks: Run Task**, and select **Rebuild Beltbound (Full)**. The PowerShell equivalent is `.\build.ps1 -Rebuild`.

The current prototype draws a square 100 x 100 grid, initially centered in the window, with 32-pixel cells and a blue highlight under the mouse. The map is 3200 x 3200 pixels. Resizing preserves the camera's bounded center and keeps cells the same pixel size. `main()` selects the grid size with `grid.setGridSize(100)`; the class default is 8. Grid rendering uses one loop to draw full-length horizontal and vertical lines. Tile occupancy and placement are not implemented yet.

## Controls and debugging

Hold **W/A/S/D** to pan the camera up/left/down/right while the game window has focus. Movement is 400 world units per second, with equal speed diagonally. The camera center stops at the map's edges; you can slide along an edge and reverse immediately. The viewport can extend beyond the map. Mouse hovering follows the panned view; the debug panel stays fixed on screen.

The grid and debug overlay start visible. Press **F3** to hide or show them together. The overlay displays FPS averaged over roughly half a second, mouse pixel and world coordinates, the hovered cell or `outside-grid`, and the grid and cell dimensions.

`Debug` also provides console logging. Startup, font-loading failure, and F3 changes produce messages; mouse movement does not print every frame. To use it with the `executableFolder` resolved in `main()`:

```cpp
Debug debug;
debug.loadFont(executableFolder / "assets/fonts/Tuffy.ttf");
debug.log("Started Beltbound.");
debug.setConsoleEnabled(false); // Silence log(); the overlay stays visible.
```

The build copies the bundled [Tuffy font and its license](assets/fonts/README.md) beside the executable. If the font is missing, one loading failure is logged and the game continues without the text overlay.

## Project layout

```text
FactoryGame/
|-- src/
|   |-- main.cpp          # Configuration and named frame/input/drawing helpers
|   |-- Grid.cpp          # Grid dimensions, coordinate lookup, and line drawing
|   `-- Debug.cpp         # Console logging and on-screen diagnostics
|-- include/
|   |-- Grid.hpp          # Grid class and method declarations
|   `-- Debug.hpp         # Debug class interface
|-- assets/fonts/         # Tuffy.ttf, license, and attribution
|-- assets/tilesets/      # Tier 1 resource atlas and local terrain
|-- docs/
|   |-- architecture.md   # Current file roles and frame lifecycle
|   |-- credits.md        # Asset credits and licenses
|   `-- CHANGELOG.md      # Development checkpoints
|-- .vscode/              # Build, IntelliSense, and debugger configuration
|-- Makefile              # Incremental compilation and linking rules
|-- build.ps1             # Run GNU Make and copy runtime files
|-- README.md
|-- .gitignore
|-- build/                # Generated executable and DLLs (ignored)
`-- vendor/               # Downloaded SFML package (ignored)
```

## Next goal
Add tile occupancy data, draw tiles from it, then use mouse clicks to change it.
The basic window, game loop, and close-event handling are in place.
Write the code yourself; ask for hints or review when needed.
https://www.sfml-dev.org/tutorials/3.1/
IMPORTANT: SFML 3 changes the event API from SFML 2; older examples need adaptation.

## Setup
The WinLibs UCRT 64-bit GCC 14.2.0 toolchain provides `C:\mingw64\bin\g++.exe` (C++17) and `C:\mingw64\bin\mingw32-make.exe`.
SFML: official 3.1.0 Windows GCC 14.2.0 64-bit package in vendor/SFML-3.1.0.
The compiler and downloaded libraries are not tracked in Git. Before building a fresh clone, use the matching 64-bit compiler and SFML links on https://www.sfml-dev.org/download/sfml/3.1.0/. Extract the compiler so that C:\mingw64\bin\g++.exe exists, and extract SFML so that vendor/SFML-3.1.0/include and vendor/SFML-3.1.0/lib exist.
Source: https://github.com/SFML/SFML/releases/tag/3.1.0

Terrain is installed locally. Obtain a licensed copy of the Adventure sheet (`The Ground v1-1.png`) and copy it to `assets/tilesets/The Ground v1-1 - Bright.png` before building. The original Adventure sheet and the bright variant use the same tile layout. Beltbound exits at startup if it cannot load that file. See [asset credits](docs/credits.md).

`.\build.ps1` recompiles only the C++ files affected by a source or header change, then relinks when needed. It copies the SFML and MinGW runtime DLLs and `assets/` beside the executable so it can run without adding the compiler to PATH or installing a font.

Build output, downloaded libraries, and old setup backups are excluded from version control.

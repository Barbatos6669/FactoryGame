# FactoryGame - SFML

A top-down factory game learning project written in C++ with SFML 3.

Working title: Beltbound. See [the development log](docs/CHANGELOG.md) for checkpoints and the next milestone, and [the architecture notes](docs/architecture.md) for how the current code fits together.

## Start
Open this folder in VS Code. Application code is in `src/main.cpp`; the grid class is split between `include/Grid.hpp` and `src/Grid.cpp`. Press Ctrl+Shift+B to build.
In the VS Code terminal run: .\build\FactoryGame.exe
Or press F5 with the `Debug Beltbound` configuration to build and start the debugger.
The current prototype draws a square 100 x 100 grid, initially centered in the window, with 32-pixel cells and a blue highlight under the mouse. The map is 3200 x 3200 pixels. Resizing preserves the camera's bounded center and keeps cells the same pixel size. `main()` selects the grid size with `grid.setGridSize(100)`; the class default is 8. Grid rendering uses one loop to draw full-length horizontal and vertical lines. Tile occupancy and placement are not implemented yet.

## Controls and debugging

Hold **W/A/S/D** to pan the camera up/left/down/right while the game window has focus. Movement is 400 world units per second, with equal speed diagonally. The camera center stops at the map's edges; you can slide along an edge and reverse immediately. The viewport can extend beyond the map. Mouse hovering follows the panned view; the debug panel stays fixed on screen.

The debug overlay starts visible in the top-right corner. Press **F3** to hide or show it. It displays FPS averaged over roughly half a second, mouse pixel and world coordinates, the hovered cell or `outside-grid`, and the grid and cell dimensions.

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
|-- docs/
|   |-- architecture.md   # Current file roles and frame lifecycle
|   `-- CHANGELOG.md      # Development checkpoints
|-- .vscode/              # Build, IntelliSense, and debugger configuration
|-- build.ps1             # Compile and copy runtime DLLs
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
Compiler: C:\mingw64\bin\g++.exe (WinLibs UCRT 64-bit GCC 14.2.0), C++17.
SFML: official 3.1.0 Windows GCC 14.2.0 64-bit package in vendor/SFML-3.1.0.
The compiler and downloaded libraries are not tracked in Git. Before building a fresh clone, use the matching 64-bit compiler and SFML links on https://www.sfml-dev.org/download/sfml/3.1.0/. Extract the compiler so that C:\mingw64\bin\g++.exe exists, and extract SFML so that vendor/SFML-3.1.0/include and vendor/SFML-3.1.0/lib exist.
Source: https://github.com/SFML/SFML/releases/tag/3.1.0
build.ps1 compiles `src/main.cpp`, `src/Grid.cpp`, and `src/Debug.cpp`, links SFML graphics, window and system, and copies the SFML and MinGW runtime DLLs and `assets/` beside the executable so it can run without adding the compiler to PATH or installing a font.

Build output, downloaded libraries, and old setup backups are excluded from version control.

# Beltbound

Beltbound is an original top-down factory game about gathering resources and building a factory. It's a learning project built with C++17 and SFML 3.1.

## Current prototype

Six starter cells sit near the center of the grid. Two show the same flowers on grass and dirt. Their definitions are in [`makeSampleTiles()`](src/Tile.cpp), and [`Tile.hpp`](include/Tile.hpp) defines the fields each cell stores.

- **1** selects grass, **2** dirt, and **3** water. **4** selects a rock and **5** flowers. The window title shows the active brush in brackets.
- **Left-click** paints the selected brush. Painting terrain keeps the existing object. Painting an object on an empty cell puts grass underneath it. **Right-click** clears the object and keeps the terrain.
- **F2** saves the map to `grass.txt` beside the executable. **F4** loads that file. A row such as `50,50 dirt flower` stores both layers. Older rows such as `50,50` load as plain grass.
- **W/A/S/D** pan the camera. **F3** toggles the grid and debug overlay. A transparent outline marks the hovered cell.

![Eight original Tier 1 resource sprites](assets/tilesets/beltbound-resources-tier-1.png)

*Original Tier 1 sprite atlas. The rock brush uses the stone pile.*

## Windows setup

1. Install **WinLibs GCC 14.2.0 (64-bit UCRT, SEH)** so the compiler and GNU Make are available at `C:\mingw64\bin\g++.exe` and `C:\mingw64\bin\mingw32-make.exe`. The VS Code debugger uses `C:\mingw64\bin\gdb.exe` from the same package. [Matching toolchain and SFML downloads](https://www.sfml-dev.org/download/sfml/3.1.0/)
2. From that download page, choose the **SFML 3.1.0 Windows package for GCC 14.2.0 MinGW (UCRT), 64-bit**. Extract it so the `include`, `lib`, and `bin` folders sit directly inside `vendor/SFML-3.1.0`.
3. Obtain a licensed copy of Backterria's Adventure terrain sheet (`The Ground v1-1.png`). Copy it to `assets/tilesets/The Ground v1-1 - Bright.png` before building. The original and bright variant share the same tile layout. Beltbound exits if it cannot load the sheet at startup. Keep the paid sheets and modified copies local. [The Ground on itch.io](https://backterria.itch.io/the-ground)

## Build and run

- Open the repository in VS Code. Press **Ctrl+Shift+B** for an incremental build, then run `.\build\FactoryGame.exe` in the terminal.
- Press **F5** with **Debug Beltbound** selected to build incrementally and launch the debugger.
- For a full rebuild, choose **Tasks: Run Task** from the Command Palette, then **Rebuild Beltbound (Full)**.

From PowerShell in the repository root, run `.\build.ps1` for an incremental build or `.\build.ps1 -Rebuild` for a full rebuild. The build copies the runtime DLLs and assets beside the executable.

## Project notes

- [Development log](docs/CHANGELOG.md)
- [Architecture notes](docs/architecture.md)
- [Asset credits and licenses](docs/credits.md)

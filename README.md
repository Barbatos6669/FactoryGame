# Beltbound

Beltbound is an isometric factory game about gathering resources and building a factory. It's a learning project built with C++17 and SFML 3.1.

## Current prototype

Six starter cells sit near the engineer. The rock to his front-right sits on three blocks: dirt, grass, then dirt again. Their definitions are in [`makeSampleTiles()`](src/Tile.cpp).

- Click a brush in the editor palette, or use **1–5** for grass, dirt, water, rock, and flowers. The selected brush stays highlighted.
- Use the palette buttons to save or load the map or toggle the grid and diagnostics. Save and load results appear on screen. To paint or zoom, point at the map outside the panels.
- **Paint (P)** changes the exposed surface or its decoration. **Raise (B)** adds the selected terrain on top, up to eight blocks per cell. **Lower (L)** removes the top block and reveals what was underneath. The base stays in place.
- **Erase (E)** removes the exposed block and anything on it. Click once to peel one layer, or hold the left button and drag to peel one layer from each cell you cross. Release and press again to peel another layer. Erasing the last block leaves an empty cell. Press **P** or select a brush to paint again.
- Click **Undo** or press **Ctrl+Z** to take back the last edit. A whole paint or erase drag counts as one edit, even if you undo before releasing the mouse. Click **Redo**, press **Ctrl+Y**, or press **Ctrl+Shift+Z** to reapply it. After undoing, changing a tile starts a new sequence of edits. **F2** keeps the history when saving. A successful **F4** load starts a fresh history.
- To paint a path, choose **Paint** and grass, dirt, water, or a library terrain stamp. Hold the left mouse button and drag over the map. Crossing a panel or map edge, moving the camera, or changing tools ends the stroke. Release and press again to start a new one.
- For a rectangular patch, hold **Shift** as you press the left mouse button on one corner. Drag to the opposite corner to preview the footprint and its dimensions in cells. Release on the map to paint, or press **Esc** to cancel. The patch keeps existing decorations and block heights. **Ctrl+Z** undoes the whole rectangle.
- To turn a grass clearing into dirt, press **P** (Paint), then **2** (Dirt). Point at the grass and press **F**. Fill follows shared edges across matching ground at the same height. It stops at cliffs, empty cells, and other surfaces. Rocks and flowers stay in place. The dock shows how many cells changed, and **Ctrl+Z** restores the whole patch.
- To reuse something on the map, hover over a cell and press **I** or **middle-click**. On a flowered grass block this picks the flowers. Hold **Shift** with either shortcut to pick the grass underneath. A successful pick switches to Paint, ready for your next click. Library stamps keep their facing and frame.
- **Left-click** a block's visible top or side to use the selected tool. **Right-click** clears the exposed decoration. Choosing Rock or Flower switches back to Paint. A decoration placed on an empty cell gets grass underneath.
- Try **2** (Dirt), then **4** (Rock), and click an empty cell to place a rock. Press **B** (Raise) and click the rock. Raise remembers Dirt and covers the rock with a dirt block. Press **Ctrl+Z** to reveal the rock again.
- **F2** saves all blocks and their decorations to `grass.txt` beside the executable. **F4** loads the map, including earlier grass-only and single-block saves.
- **W/A/S/D** walk the blue woodland engineer. Hold two neighboring directions to walk diagonally. Release the keys to idle facing the last direction of travel.
- The camera stays centered on the engineer as he walks. **Arrow keys** let you look around while standing still. Your next step centers the camera on him again. Without the optional engineer sheets, both WASD and the arrows pan the editor.
- **Scroll up** to zoom in, up to 1.5× after four wheel notches. **Scroll down** to return to the original 1× view.
- **F3** toggles the grid and debug overlay. A yellow diamond marks the hovered cell.

### Developer asset library

Click **Library** or press **Tab**. Type a name, material, team color or biome. The category menu and **Placeable only** filter narrow the results. Scroll or use **Prev** and **Next** to browse. Select a card to inspect it, choose its facing and frame, then click **Use stamp**. A translucent preview follows the pointer. Left-click the map to place it. **Esc** closes the library while browsing. On the map, **Esc** clears the active stamp.

With the optional Tiny Warfare pack installed, search for **snow ice**, select a terrain stamp and paint a cell. Open the library again, search for **copper**, and stamp the deposit on that same cell. Right-click removes the deposit and leaves the snow. Raise can add another block of the selected library terrain. **F2** and **F4** save and restore both choices on each layer.

The public checkout includes all eight Beltbound resource sprites and the original eight-direction field engineer as static stamps. You can also inspect the Kenney UI artwork and font. Installing Tiny Warfare adds terrain, buildings, walls, units, vehicles, effects and props, along with pack references such as team masks and terrain guides. The library indexes the locally installed files in the background the first time they are needed.

![Eight original Tier 1 resource sprites](assets/tilesets/beltbound-resources-tier-1.png)

*Beltbound's original Tier 1 resource art.*

## Windows setup

1. Install **WinLibs GCC 14.2.0 (64-bit UCRT, SEH)** so the compiler and GNU Make are available at `C:\mingw64\bin\g++.exe` and `C:\mingw64\bin\mingw32-make.exe`. The VS Code debugger uses `C:\mingw64\bin\gdb.exe` from the same package. [Matching toolchain and SFML downloads](https://www.sfml-dev.org/download/sfml/3.1.0/)
2. From that download page, choose the **SFML 3.1.0 Windows package for GCC 14.2.0 MinGW (UCRT), 64-bit**. Extract it so the `include`, `lib`, and `bin` folders sit directly inside `vendor/SFML-3.1.0`.
3. The editor includes basic isometric terrain and objects. For the textured terrain, rock and animated engineer, extract the optional purchased `tiny-warfare-paid-sprites.zip` into `assets/mythryl-studio/tiny-warfare/` with `sprites.json`, `README.txt`, and `LICENSE.txt` at its root. Keep the pack's subfolders intact. The normal build copies the assets beside the executable. [Tiny Warfare on itch.io](https://mythrylstudio.itch.io/tiny-warfare)

## Build and run

- Open the repository in VS Code. Press **Ctrl+Shift+B** for an incremental build, then run `.\build\FactoryGame.exe` in the terminal.
- Press **F5** with **Debug Beltbound** selected to build incrementally and launch the debugger.
- For a full rebuild, choose **Tasks: Run Task** from the Command Palette, then **Rebuild Beltbound (Full)**.

From PowerShell in the repository root, run `.\build.ps1` for an incremental build or `.\build.ps1 -Rebuild` for a full rebuild. The build copies the runtime DLLs and assets beside the executable.

## Project notes

- [Development log](docs/CHANGELOG.md)
- [Architecture notes](docs/architecture.md)
- [Terrain and feature catalog](docs/terrain-catalog.md)
- [Asset credits and licenses](docs/credits.md)

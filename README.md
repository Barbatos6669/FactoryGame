# FactoryGame - SFML

A top-down factory game learning project written in C++ with SFML 3.

Working title: Beltbound. See [CHANGELOG.md](CHANGELOG.md) for development checkpoints and the next milestone.

## Start
Open this folder in VS Code. Edit main.cpp. Press Ctrl+Shift+B to build.
In the VS Code terminal run: .\build\FactoryGame.exe
The current prototype opens a window and draws an 8 x 8 grid. Tile occupancy data is defined but is not connected to drawing or input yet.

## Next goal
Draw tiles according to their stored occupancy, then use mouse clicks to change them.
The basic window, game loop, and close-event handling are in place.
Write the code yourself; ask for hints or review when needed.
https://www.sfml-dev.org/tutorials/3.1/
IMPORTANT: SFML 3 changes the event API from SFML 2; older examples need adaptation.

## Setup
Compiler: C:\mingw64\bin\g++.exe (64-bit GCC 14.2.0), C++17.
SFML: official 3.1.0 Windows GCC 14.2.0 64-bit package in vendor/SFML-3.1.0.
Downloaded libraries are not tracked in Git. Before building a fresh clone, download the matching SFML package and extract it so that vendor/SFML-3.1.0/include and vendor/SFML-3.1.0/lib exist.
Source: https://github.com/SFML/SFML/releases/tag/3.1.0
build.ps1 links graphics, window and system, and copies the three DLLs beside the executable.

Build output, downloaded libraries, and old setup backups are excluded from version control.

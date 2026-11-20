/**
 * @file main.cpp
 * @brief Main entry point for the game application.
 *
 * This file contains the main function that initializes the game,
 * sets up resources, and enters the main game loop. It uses the SFML
 * library for graphics rendering and window management.
 *
 * @author Barbatos6669
 * @version 0.1.0
 * @copyright (c) 2026 Barbatos6669. All rights reserved.
 */

#include <SFML/Graphics.hpp>
#include <iostream>


constexpr unsigned int WINDOW_WIDTH = 1920;  ///< Initial window width in pixels.
constexpr unsigned int WINDOW_HEIGHT = 1080; ///< Initial window height in pixels. 

struct Tile
{
    bool occupied = false; ///< Indicates whether the tile is occupied.
};

void setup(sf::RenderWindow& window)
{
    //create a window
    window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Beltbound");
    window.setFramerateLimit(60);
}

void loop(sf::RenderWindow& window)
{

    Tile grid[8][8]; // 8x8 grid of tiles
    
    // Main game loop
    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

        window.clear();

        // Draw your game objects here
        sf::RectangleShape horizontalLine;
        sf::RectangleShape verticalLine;
        sf::RectangleShape gridFrame;

        horizontalLine.setSize({32.f, 1.f});
        horizontalLine.setOutlineThickness(0.f);
        horizontalLine.setPosition({0.f, 0.f});
        horizontalLine.setFillColor(sf::Color::White);

        verticalLine.setSize({1.f, 32.f});
        verticalLine.setOutlineThickness(0.f);
        verticalLine.setPosition({0.f, 0.f});
        verticalLine.setFillColor(sf::Color::White);

        int gridSize = 8;
        int cell = 32;
        int column = gridSize;
        int row = gridSize;

        int mapWidth = column * cell;
        int mapHeight = row * cell;

        gridFrame.setSize({static_cast<float>(mapWidth), static_cast<float>(mapHeight)});
        gridFrame.setFillColor(sf::Color::Transparent);
        gridFrame.setOutlineThickness(1.f);
        gridFrame.setOutlineColor(sf::Color::Green);
        gridFrame.setPosition({0.f, 0.f});

        window.draw(gridFrame);

        for (int a = 0; a < column; a++)
        {
            for (int b = 0; b < row; b++)
            {
                horizontalLine.setPosition({static_cast<float>(a * cell), static_cast<float>(b * cell)});
                verticalLine.setPosition({static_cast<float>(a * cell), static_cast<float>(b * cell)});
                window.draw(horizontalLine);
                window.draw(verticalLine);
            }
        }


        window.draw(horizontalLine);
        window.draw(verticalLine);
        window.display();
    }
}

int main()
{
    sf::RenderWindow window;
    
    std::cout << "Hello, SFML! we are making a game!" << std::endl;

    setup(window);
    loop(window);

    return 0;
}

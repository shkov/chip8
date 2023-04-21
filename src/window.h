#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace chip8
{

class Window : private sf::RenderWindow
{
  private:
    using Base = sf::RenderWindow;

  public:
    Window() : Base{ sf::VideoMode{ { 1280, 800 } }, "Chip8" }
    {
        clear(sf::Color::White);
        display();
    };

    bool IsOpen()
    {
        if (closed_)
            return false;

        sf::Event event;
        bool ok{ Base::pollEvent(event) };
        if (ok && event.type == sf::Event::Closed)
        {
            close();
            closed_ = true;
            return false;
        }

        return true;
    }

    void Draw()
    {
        if (closed_)
            return;

        clear(sf::Color::White);

        sf::RectangleShape pixel{ sf::Vector2f(10, 10) };
        pixel.setFillColor(sf::Color::Red);

        pixel.setPosition(sf::Vector2f(100, 100));

        draw(pixel);

        display();
    };

  private:
    bool closed_{ false };
};

} // namespace chip8

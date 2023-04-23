#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>

namespace chip8
{

class Window : private sf::RenderWindow
{
  private:
    using Base = sf::RenderWindow;

    static inline const size_t kPixelSize{ 20 };
    static inline const size_t kScreenWidth{ kPixelSize * 64 + kPixelSize - 1 };
    static inline const size_t kScreenHeight{ kPixelSize * 32 + kPixelSize - 1 };

  public:
    Window() : Base{ sf::VideoMode{ { kScreenWidth, kScreenHeight } }, "Chip8" }
    {
        clear(sf::Color::Black);
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

    std::optional<sf::Keyboard::Key> WaitKeyPress()
    {
        if (closed_)
            return std::nullopt;

        sf::Event event;
        if (waitEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                close();
                closed_ = true;
                return std::nullopt;
            }
            if (event.type == sf::Event::KeyPressed)
                return event.key.code;
        }

        return sf::Keyboard::Unknown;
    };

    void Draw(const std::array<std::array<uint8_t, 64>, 32>& screen)
    {
        if (closed_)
            return;

        clear(sf::Color::Black);

        float position_y{ 0 };
        float position_x{ 0 };

        for (size_t y = 0; y < 32; ++y)
        {
            for (size_t x = 0; x < 64; ++x)
            {
                sf::RectangleShape pixel{ sf::Vector2f(kPixelSize, kPixelSize) };
                pixel.setFillColor(screen[y][x] ? sf::Color::White : sf::Color::Black);
                pixel.setPosition(sf::Vector2f(position_x, position_y));
                draw(pixel);
                position_x += kPixelSize + 1;
            }
            position_x = 0;
            position_y += kPixelSize + 1;
        }

        display();
    };

  private:
    bool closed_{ false };
};

} // namespace chip8

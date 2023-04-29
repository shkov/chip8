#pragma once

#include <SFML/Audio.hpp>
#include <exception>
#include <string>

namespace chip8 {

class Speaker {
 public:
  explicit Speaker(const std::string& filename) {
    if (!buffer_.loadFromFile(filename))
      throw std::invalid_argument{ "failed to load sound buffer from file" };

    sound_.setBuffer(buffer_);
  };

  void Play() { sound_.play(); }

 private:
  sf::SoundBuffer buffer_;
  sf::Sound sound_;
};

}  // namespace chip8

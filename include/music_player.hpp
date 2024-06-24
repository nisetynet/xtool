#pragma once

#include <miniaudio.h>

#include <playlist.hpp>

class MusicPlayer {
public:
  // MusicPlayer() = delete;
  MusicPlayer();
  ~MusicPlayer();
  [[nodiscard]] bool play(MusicEntry const &music_entry);

private:
  [[nodiscard]] bool stop();

private:
  ma_device m_ma_device;
  ma_decoder m_ma_decoder;
  bool m_is_playing = false;
};
#pragma once
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <random>
#include <syncstream>
#include <toml.hpp>
#include <unordered_map>
#include <unordered_set>
#include <spdlog/spdlog.h>

using music_entry =
    std::tuple<std::uint64_t, std::filesystem::path,
               std::pair<std::uint64_t, std::uint64_t>,
               std::optional<std::pair<std::uint64_t, std::uint64_t>>>;

struct Playlist {
public:
  Playlist(std::string_view const playlist_config_toml_file_path);

  std::optional<music_entry> random_music_for(std::uint16_t const music_id);

private:
  // unique music id to music entry
  std::unordered_map<std::uint64_t, music_entry> m_music_map;

  // brawl music id to unique music ids
  std::unordered_map<std::uint16_t, std::vector<std::uint64_t>>
      m_brawl_music_id_to_unique_ids_map;
};

std::size_t time_seed_rand(std::size_t const l, std::size_t const r);
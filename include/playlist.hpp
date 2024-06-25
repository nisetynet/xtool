#pragma once
#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <syncstream>
#include <toml.hpp>
#include <unordered_map>
#include <unordered_set>

using UniqueMusicID = std::uint64_t;
using BrawlMusicID = std::uint64_t;

struct MusicEntry {
  UniqueMusicID unique_music_id;
  std::filesystem::path music_file_path;
  std::pair<std::uint64_t, std::uint64_t> start_end_offsets;
  std::optional<std::pair<std::uint64_t, std::uint64_t>> loop_start_end_offsets;
};

struct Playlist {
public:
  Playlist(std::filesystem::path const &playlist_config_toml_file_path);

  std::optional<MusicEntry> random_music_for(BrawlMusicID const brawl_music_id,
                                             std::uint32_t const seed);

public:
  std::unordered_map<UniqueMusicID, MusicEntry> const &
  music_map() const noexcept;
  std::unordered_map<BrawlMusicID, std::vector<UniqueMusicID>> const &
  brawl_music_id_to_unique_ids_map() const noexcept;

private:
  // unique music id to music entry
  std::unordered_map<UniqueMusicID, MusicEntry> m_music_map;

  // brawl music id to unique music ids
  std::unordered_map<BrawlMusicID, std::vector<UniqueMusicID>>
      m_brawl_music_id_to_unique_ids_map;
};

std::size_t seed_rand(std::size_t const l, std::size_t const r,
                           std::uint32_t const additional_seed);
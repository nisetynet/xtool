#pragma once
#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <random>
#include <spdlog/spdlog.h>
#include <syncstream>
#include <toml++/toml.hpp>
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

struct PlaylistEntry {
  std::string name;
  std::vector<BrawlMusicID> target_music_ids;
  std::vector<UniqueMusicID> music_entries;
};

struct Playlist {
public:
  Playlist(std::filesystem::path const &playlist_config_toml_file_path);

  [[nodiscard]] std::optional<MusicEntry>
  random_music_for_with_g_mtRand_seed(BrawlMusicID const brawl_music_id,
                                      std::uint32_t const seed) const;

  [[nodiscard]] std::optional<MusicEntry>
  random_music_for_with_std_random_device(
      BrawlMusicID const brawl_music_id) const;

public:
  [[nodiscard]] std::unordered_map<UniqueMusicID, MusicEntry> const &
  music_map() const noexcept;

  [[nodiscard]] std::unordered_map<BrawlMusicID,
                                   std::shared_ptr<PlaylistEntry>> const &
  brawl_music_id_to_playlist_entry_map() const noexcept;

private:
  // unique music id to music entry
  std::unordered_map<UniqueMusicID, MusicEntry> m_music_map;

  std::vector<std::shared_ptr<PlaylistEntry>> m_playlist_entries;
  std::unordered_map<BrawlMusicID, std::shared_ptr<PlaylistEntry>>
      m_brawl_music_id_to_playlist_entry_map;
};

std::size_t seed_rand(std::size_t const l, std::size_t const r,
                      std::uint32_t const additional_seed);
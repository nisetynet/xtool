#include "playlist.hpp"
#include <inspection.hpp>
#include <unordered_map>
#include <unordered_set>

void inspect_config(std::string_view const config_file_path) {

  spdlog::info("Inspect config");
  Playlist playlist(config_file_path);

  auto const music_map = playlist.music_map();
  auto const brawl_music_id_to_playlist_entry_map =
      playlist.brawl_music_id_to_playlist_entry_map();

  // find unused musics
  std::unordered_map<UniqueMusicID, MusicEntry> unused_musics = music_map;

  for (auto const &[ignore_, playlist_entry] :
       brawl_music_id_to_playlist_entry_map) {
    for (auto const unique_music_id : playlist_entry->music_entries) {
      // remove used music id
      unused_musics.erase(unique_music_id);
    }
  }

  spdlog::info("********** Inspection Result **********");
  spdlog::info("Total music entries={}", music_map.size());
  spdlog::info("Total unused music entries={}", unused_musics.size());

  for (auto const &[k, v] : unused_musics) {
    assert(k == v.unique_music_id);
    spdlog::info("[Unused music entry] unique music id={:#x}, file='{}'", k,
                 v.music_file_path.string());
  }
}

void inspect_musics(std::string_view const config_file_path,
                    std::vector<UniqueMusicID> const &unique_music_ids) {
  spdlog::info("Inspect registered musics");
  Playlist pl(config_file_path);

  auto const &music_map = pl.music_map();
  auto const &brawl_music_id_to_playlist_entry_map =
      pl.brawl_music_id_to_playlist_entry_map();

  for (auto const unique_music_id : unique_music_ids) {
    if (!music_map.contains(unique_music_id)) {
      spdlog::error("Unique music id {} is not registered.", unique_music_id);
      continue;
    }
  }
}
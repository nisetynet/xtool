#include <inspection.hpp>
#include <memory>
#include <playlist.hpp>
#include <unordered_map>

void inspect_config(std::string_view const config_file_path) {

  spdlog::info("Inspect config");
  Playlist playlist(config_file_path);

  auto const &music_map = playlist.music_map();
  auto const &brawl_music_id_to_playlist_entry_map =
      playlist.brawl_music_id_to_playlist_entry_map();

  auto const &playlist_entries = playlist.playlist_entries();

  spdlog::info("********** Inspection Result **********");

  spdlog::info("Playlists");
  // print all playlist entries
  for (auto const &pe : playlist_entries) {
    spdlog::info("Playlist {}, {} target ids, {} musics.", pe->name,
                 pe->target_music_ids.size(), pe->music_entries.size());
    for (auto const unique_music_id : pe->music_entries) {
      auto const &music = music_map.at(unique_music_id);
      spdlog::info(fmt::format("\tMusic:{}", music));
    }
  }

  // find unused musics
  std::unordered_map<UniqueMusicID, MusicEntry> unused_musics = music_map;

  for (auto const &[ignore_, playlist_entry] :
       brawl_music_id_to_playlist_entry_map) {
    for (auto const unique_music_id : playlist_entry->music_entries) {
      // remove used music id
      unused_musics.erase(unique_music_id);
    }
  }

  spdlog::info("Total music entries={}", music_map.size());
  spdlog::info("Total unused music entries={}", unused_musics.size());

  for (auto const &[k, v] : unused_musics) {
    assert(k == v.unique_music_id);
    spdlog::info(fmt::format("[Unused music entry] {}", v));
  }
}

void inspect_musics(std::string_view const config_file_path,
                    std::vector<UniqueMusicID> const &unique_music_ids) {
  spdlog::info("Inspect registered musics");
  Playlist pl(config_file_path);

  auto const &playlist_entries = pl.playlist_entries();

  // construct UniqueMusicID -> PlaylistEntry multi map
  std::unordered_multimap<UniqueMusicID, std::shared_ptr<PlaylistEntry>> map;

  for (auto const &pe : playlist_entries) {
    for (auto const id : pe->music_entries) {
      map.insert(std::make_pair(id, pe));
    }
  }

  // inspect music usages
  for (auto const unique_music_id : unique_music_ids) {
    if (!map.contains(unique_music_id)) {
      spdlog::warn("Unique music id {} is not registered.", unique_music_id);
      continue;
    }

    spdlog::info("Unique music id {} usages", unique_music_id);
    auto const range = map.equal_range(unique_music_id);
    for (auto it = range.first; it != range.second; ++it) {
      spdlog::info("\tPlaylist {}", it->second->name);
    }
  }
}
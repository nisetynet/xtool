#include <inspection.hpp>
#include <unordered_map>
#include <unordered_set>

void inspect_config(Playlist const &playlist) {
  auto const music_map = playlist.music_map();
  auto const brawl_music_id_to_unique_ids_map =
      playlist.brawl_music_id_to_unique_ids_map();

  // find unused musics
  std::unordered_map<UniqueMusicID, MusicEntry> unused_musics = music_map;

  for (auto const &[ignore_, unique_music_ids] :
       brawl_music_id_to_unique_ids_map) {
    for (auto const unique_music_id : unique_music_ids) {
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
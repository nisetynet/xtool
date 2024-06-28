#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <playlist.hpp>
#include <random>
#include <stdexcept>
#include <toml++/impl/array.hpp>
#include <unordered_map>

template <typename T, typename U>
auto fmt::formatter<std::pair<T, U>>::format(
    std::pair<T, U> const &pair, auto &ctx) const -> format_context::iterator {
  return fmt::format_to(ctx.out(), "({}, {})", pair.first, pair.second);
}

auto fmt::formatter<MusicEntry>::format(MusicEntry const &me,
                                        format_context &ctx) const
    -> format_context::iterator {
  return fmt::format_to(
      ctx.out(),
      "id={}, file={}, start offset={}, end offset={}, loop offsets={}",
      me.unique_music_id, me.music_file_path.string(),
      me.start_end_offsets.first, me.start_end_offsets.second,
      me.loop_start_end_offsets);
}

Playlist::Playlist(
    std::filesystem::path const &playlist_config_toml_file_path) {
  toml::parse_result result =
      toml::parse_file(playlist_config_toml_file_path.string());
  if (!result.is_table()) {
    throw std::runtime_error("Invalid toml file, expected table element.");
  }
  auto table = (toml::table)(result);

  // construct map
  std::unordered_map<UniqueMusicID, MusicEntry> music_map{};

  auto musics = table["musics"]["musics"];

  auto musics_array = musics.as_array();
  musics_array->for_each([&](auto &elm) {
    if (!elm.is_array()) {
      throw std::runtime_error("Expected an array in musics.musics table.");
    }
    auto const music_infos = elm.as_array();
    auto const music_unique_id = music_infos->get(0);
    auto const music_file_path = music_infos->get(1);
    auto const music_start_offset = music_infos->get(2); // -1 for default
    auto const music_end_offset = music_infos->get(3);   // -1 for default
    auto const loop_begin_offset = music_infos->get(4);  // optional
    auto const loop_end_offset = music_infos->get(5);    // optional

    auto const is_music_unique_id_available =
        music_unique_id && music_unique_id->is_integer();
    auto const is_music_file_path_available =
        music_file_path && music_file_path->is_string();
    auto const is_music_start_offset_available =
        music_start_offset && music_start_offset->is_integer();
    auto const is_music_end_offset_available =
        music_end_offset && music_end_offset->is_integer();

    if (!is_music_unique_id_available) {
      throw std::runtime_error("Music id not found or invalid.");
    }

    if (!is_music_file_path_available) {
      throw std::runtime_error("Music flle path not found or invalid.");
    }

    if (!is_music_start_offset_available) {
      throw std::runtime_error("Music start offset not found or invalid.");
    }

    if (!is_music_end_offset_available) {
      throw std::runtime_error("Music end offset not found or invalid.");
    }

    auto const is_loop_begin_offset_available =
        loop_begin_offset && loop_begin_offset->is_integer();
    auto const is_loop_end_offset_available =
        loop_end_offset && loop_end_offset->is_integer();

    if ((is_loop_begin_offset_available && !is_loop_end_offset_available) ||
        (!is_loop_begin_offset_available && is_loop_end_offset_available)) {
      throw std::runtime_error("Only begin or end loop offset was found.");
    }

    // construct music entry
    std::int64_t const music_unique_id_value =
        music_unique_id->as_integer()->get();

    auto const music_file_path_value =
        std::filesystem::path(music_file_path->as_string()->get());

    // Detect some characters that cause problems on windows
    // Incomplete but helps.
    for (auto const c : music_file_path_value.string()) {
      static std::unordered_set<char> ALLOW_CHARS{'.', ' ', '/'};
      if (ALLOW_CHARS.contains(c)) {
        continue;
      }
      if (!std::isgraph(static_cast<unsigned char>(c))) {
        throw std::runtime_error(
            fmt::format("Invalid character \"{}\" found in {}", c,
                        music_file_path_value.string().c_str()));
      }

      if (std::iscntrl(static_cast<unsigned char>(c))) {
        throw std::runtime_error(
            fmt::format("Invalid control character {} found in {}", c,
                        music_file_path_value.string().c_str()));
      }
    }

    if (std::filesystem::is_symlink(music_file_path_value)) {
      throw std::runtime_error(
          fmt::format("{} is symlink! (We don't support symlink.)",
                      music_file_path_value.string()));
    }

    if (!std::filesystem::exists(music_file_path_value)) {
      throw std::runtime_error(
          fmt::format("{} does not exist!", music_file_path_value.string()));
    }

    if (!std::filesystem::is_regular_file(music_file_path_value)) {
      throw std::runtime_error(fmt::format("{} is not a regular file!",
                                           music_file_path_value.string()));
    }

    auto const music_start_offset_value =
        music_start_offset->as_integer()->get();
    auto const music_end_offset_value = music_end_offset->as_integer()->get();

    if (music_start_offset_value == -1 && music_end_offset_value != -1) {
      if (music_end_offset_value < 0) {
        throw std::runtime_error("Invalid music end offset.");
      }
    }

    if (music_start_offset_value != -1 && music_end_offset_value == -1) {
      if (music_start_offset_value < 0) {
        throw std::runtime_error("Invalid music start offset.");
      }
    }

    MusicEntry entry;
    if (is_loop_begin_offset_available && is_loop_end_offset_available) {
      auto const loop_begin = loop_begin_offset->as_integer()->get();
      auto const loop_end = loop_end_offset->as_integer()->get();
      if (loop_end <= loop_begin) {
        throw std::runtime_error(fmt::format(
            "Invalid loop offsets: begin={}, end={}", loop_begin, loop_end));
      }

      entry = {static_cast<UniqueMusicID>(music_unique_id_value),
               music_file_path_value,
               std::make_pair(music_start_offset_value, music_end_offset_value),
               std::make_pair(loop_begin, loop_end)};
    } else {
      entry = {static_cast<UniqueMusicID>(music_unique_id_value),
               music_file_path_value,
               std::make_pair(music_start_offset_value, music_end_offset_value),
               std::nullopt};
    }

    if (music_map.count(music_unique_id_value)) {
      throw std::runtime_error(fmt::format(
          "Duplicated music entry for id {:#x}", music_unique_id_value));
    }

    music_map[music_unique_id_value] = entry;
  });
  m_music_map = music_map;

  spdlog::info("Found {} musics!", m_music_map.size());

  // for debugging
  /*
  for (auto &[k, v] : m_music_map) {
   spdlog::info(
        << fmt::format("unique music id: {}, music file path: {}", k,
                       std::get<1>(v).string().c_str());
  }
  */

  std::unordered_map<BrawlMusicID, std::vector<UniqueMusicID>>
      brawl_music_id_to_unique_ids_map{};

  std::vector<std::shared_ptr<PlaylistEntry>> playlist_entries;

  // read other tables
  for (auto const &entry : table) {
    if (entry.first.str() == "musics") {
      continue;
    }
    spdlog::info("Found playlist: {}", entry.first.str());
    auto const &node = entry.second;

    if (!node.is_table()) {
      throw std::runtime_error("Expected table node, invalid config.");
    }

    auto const playlist_table = node.as_table();
    assert(playlist_table);

    spdlog::info("Reading unique music ids for table: {}", entry.first.str());

    // playlist musics
    auto const musics = playlist_table->get("musics");
    if (!musics) {
      throw std::runtime_error(fmt::format(
          "Playlist {}, a musics table not found.", entry.first.str()));
    }
    if (!musics->is_array()) {
      throw std::runtime_error(fmt::format(
          "Playlist {}, expected an array for musics.", entry.first.str()));
    }
    auto const music_array = musics->as_array();
    assert(music_array);

    auto unique_music_ids = std::vector<std::uint64_t>{};
    unique_music_ids.reserve(512);

    music_array->for_each([&](auto &elm) {
      if (!elm.is_integer()) {
        throw std::runtime_error(
            fmt::format("Playlist {}, expected an integer for music id.",
                        entry.first.str()));
      }
      auto const unique_music_id = elm.as_integer()->get();
      unique_music_ids.push_back(unique_music_id);
    });

    spdlog::info("Found {} musics for table: {}", unique_music_ids.size(),
                 entry.first.str());

    spdlog::info("Reading target ids for table: {}", entry.first.str());

    auto const target_ids = playlist_table->get("target_ids");
    if (!target_ids) {
      throw std::runtime_error(
          fmt::format("Playlist {}, target_ids not found.", entry.first.str()));
    }
    if (!target_ids->is_array()) {
      throw std::runtime_error(fmt::format(
          "Playlist {}, expected an array for target_ids.", entry.first.str()));
    }
    auto const target_id_array = target_ids->as_array();
    assert(target_id_array);

    // verify target ids
    std::vector<BrawlMusicID> ids;
    ids.reserve(target_id_array->size());

    target_id_array->for_each([&](auto &elm) {
      if (!elm.is_integer()) {
        throw std::runtime_error(
            fmt::format("Playlist {}, expected an integer for target id.",
                        entry.first.str()));
      }
      auto const target_id = elm.as_integer()->get();

      if (brawl_music_id_to_unique_ids_map.count(target_id)) {
        throw std::runtime_error(
            fmt::format("Duplicated target id: {:#x}", target_id));
      }
      brawl_music_id_to_unique_ids_map[target_id] = unique_music_ids;

      ids.push_back(target_id);
    });

    auto playlist_entry = std::make_shared<PlaylistEntry>(PlaylistEntry{
        entry.first.str().data(),
        ids,
        unique_music_ids,
    });

    playlist_entries.push_back(playlist_entry);

    for (auto const target_id : ids) {
      m_brawl_music_id_to_playlist_entry_map.insert(
          std::make_pair(target_id, playlist_entry));
    }

    spdlog::info("Loaded playlist {} successfully!", entry.first.str());
  }

  m_playlist_entries = playlist_entries;

  // for debugging
  /*
  for (auto [k, v] : m_brawl_music_id_to_unique_ids_map) {
    for (auto id : v) {
     spdlog::info(
          "brawl music id: {:#x}, unique music id: {:#x}", k, id);
    }
  }
  */
}

std::optional<MusicEntry>
Playlist::random_music_for_with_g_mtRand_seed(BrawlMusicID const music_id,
                                              std::uint32_t const seed) const {
  if (!m_brawl_music_id_to_playlist_entry_map.contains(music_id)) {
    return std::nullopt;
  }

  auto const &playlist_entry =
      m_brawl_music_id_to_playlist_entry_map.at(music_id);

  if (playlist_entry->music_entries.size() == 0) {
    return std::nullopt;
  }
  // choose one randomly

  spdlog::info("Using seed {:#x}", seed);
  spdlog::info("Found PlaylistEntry {}, {} musics for brawl music id {:#x}.",
               playlist_entry->name, playlist_entry->music_entries.size(),
               music_id);
  auto const random_index =
      seed_rand(0, playlist_entry->music_entries.size() - 1, seed);
  auto const unique_music_id = playlist_entry->music_entries[random_index];

  if (!m_music_map.contains(unique_music_id)) {
    return std::nullopt;
  }

  auto entry = m_music_map.at(unique_music_id);
  return entry;
}

std::optional<MusicEntry> Playlist::random_music_for_with_std_random_device(
    BrawlMusicID const music_id) const {
  if (!m_brawl_music_id_to_playlist_entry_map.contains(music_id)) {
    return std::nullopt;
  }

  auto const &playlist_entry =
      m_brawl_music_id_to_playlist_entry_map.at(music_id);

  if (playlist_entry->music_entries.size() == 0) {
    return std::nullopt;
  }
  // choose one randomly

  spdlog::warn("Using std::random_device.");
  spdlog::info("Found PlaylistEntry {}, {} musics for brawl music id {:#x}.",
               playlist_entry->name, playlist_entry->music_entries.size(),
               music_id);

  std::random_device rd;
  std::mt19937 rng(rd());

  std::uniform_int_distribution<std::size_t> dist(
      0, playlist_entry->music_entries.size() - 1);
  std::size_t random_index = dist(rng);
  auto const unique_music_id = playlist_entry->music_entries[random_index];

  if (!m_music_map.contains(unique_music_id)) {
    return std::nullopt;
  }

  auto entry = m_music_map.at(unique_music_id);
  return entry;
}

std::unordered_map<UniqueMusicID, MusicEntry> const &
Playlist::music_map() const noexcept {
  return m_music_map;
}

std::vector<std::shared_ptr<PlaylistEntry>> const &
Playlist::playlist_entries() const noexcept {
  return m_playlist_entries;
}

std::unordered_map<BrawlMusicID, std::shared_ptr<PlaylistEntry>> const &
Playlist::brawl_music_id_to_playlist_entry_map() const noexcept {
  return m_brawl_music_id_to_playlist_entry_map;
}

// メモ:
// ネットプレイで途中からツールを起動した人(クライアント)も同期できるように、毎回現在時刻でシードを生成して乱数を一度だけ生成してシードやdistributionは使い回さない
// 現在の実装ではadditional_seedにゲーム内のseedを拝借して利用している
// ゲーム内のseedが更新されるタイミングを観察した限り、ギリクライアント間で同期したまま使えそうな感じがした
std::size_t seed_rand(std::size_t const l, std::size_t const r,
                      std::uint32_t const additional_seed) {
  assert(l <= r);
  // seed
  /*
  Old time based seed code
  bad rand quality
    std::time_t now = std::time(nullptr);

    auto const tm = std::gmtime(&now);
    if (!tm) {
      throw std::runtime_error("Failed to convert std::time_t to UTC");
    }

    int minutes_since_epoch = (now / 60);

    std::uint32_t const seed = minutes_since_epoch ^ additional_seed;

    spdlog::info("seed {:#x} = minutes_since_epoch({:#x}) ^ {:#x}", seed,
                 minutes_since_epoch, constant);
  */
  // generate
  std::mt19937 rng(additional_seed);
  std::uniform_int_distribution<std::size_t> dist(l, r);
  std::size_t v = dist(rng);
  return v;
}
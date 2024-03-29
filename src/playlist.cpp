#include <cassert>
#include <exception>
#include <playlist.hpp>

Playlist::Playlist(std::string_view const playlist_config_toml_file_path) {
  toml::parse_result result = toml::parse_file(playlist_config_toml_file_path);
  if (!result.is_table()) {
    exit(-1);
  }
  auto table = (toml::table)(result);

  // construct map
  std::unordered_map<std::uint64_t, music_entry> music_map{};
  std::unordered_map<std::uint16_t, std::vector<std::uint64_t>>
      music_id_music_map_key_map{};

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
      throw std::runtime_error(
          fmt::format("{} is not a music file!", music_file_path_value.string()));
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

    music_entry entry;
    if (is_loop_begin_offset_available && is_loop_end_offset_available) {
      auto const loop_begin = loop_begin_offset->as_integer()->get();
      auto const loop_end = loop_end_offset->as_integer()->get();
      if (loop_end <= loop_begin) {
        throw std::runtime_error(fmt::format(
            "Invalid loop offsets: begin={}, end={}", loop_begin, loop_end));
      }

      entry = std::make_tuple(
          music_unique_id_value, music_file_path_value,
          std::make_pair(music_start_offset_value, music_end_offset_value),
          std::make_pair(loop_begin, loop_end));
    } else {
      entry = std::make_tuple(
          music_unique_id_value, music_file_path_value,
          std::make_pair(music_start_offset_value, music_end_offset_value),
          std::nullopt);
    }

    if (music_map.count(music_unique_id_value)) {
      throw std::runtime_error(fmt::format(
          "Duplicated music entry for id {:#x}\n", music_unique_id_value));
    }

    music_map[music_unique_id_value] = entry;
  });
  m_music_map = music_map;

  std::osyncstream(std::cout)
      << fmt::format("[+] Found {} musics!\n", m_music_map.size());

  // for debugging
  /*
  for (auto &[k, v] : m_music_map) {
    std::osyncstream(std::cout)
        << fmt::format("unique music id: {}, music file path: {}\n", k,
                       std::get<1>(v).string().c_str());
  }
  */

  std::unordered_map<std::uint16_t, std::vector<std::uint64_t>>
      brawl_music_id_to_unique_ids_map{};

  // read other tables
  for (auto const &entry : table) {
    if (entry.first.str() == "musics") {
      continue;
    }
    std::osyncstream(std::cout)
        << fmt::format("[+] Found playlist: {}\n", entry.first.str());
    auto const &node = entry.second;

    if (!node.is_table()) {
      throw std::runtime_error("Expected table node, invalid config.");
    }

    auto const playlist_table = node.as_table();
    assert(playlist_table);

    std::osyncstream(std::cout) << fmt::format(
        "[+] Reading unique music ids for table: {}\n", entry.first.str());
    auto unique_music_ids = std::vector<std::uint64_t>{};

    auto const musics = playlist_table->get("musics");
    if (!musics) {
      throw std::runtime_error("musics table not found.");
    }
    if (!musics->is_array()) {
      throw std::runtime_error("Expected musics array.");
    }
    auto const music_array = musics->as_array();
    assert(music_array);

    music_array->for_each([&](auto &elm) {
      if (!elm.is_integer()) {
        throw std::runtime_error("Expected an integer.");
      }
      auto const unique_music_id = elm.as_integer()->get();
      unique_music_ids.push_back(unique_music_id);
    });

    std::osyncstream(std::cout) << fmt::format(
        "[+] Reading target ids for table: {}\n", entry.first.str());

    auto const target_ids = playlist_table->get("target_ids");
    if (!target_ids) {
      throw std::runtime_error("target_ids not found.");
    }
    if (!target_ids->is_array()) {
      throw std::runtime_error("Expected an array.");
    }
    auto const target_id_array = target_ids->as_array();
    assert(target_id_array);

    target_id_array->for_each([&](auto &elm) {
      if (!elm.is_integer()) {
        throw std::runtime_error("Expected an integer.");
      }
      auto const target_id = elm.as_integer()->get();

      if (brawl_music_id_to_unique_ids_map.count(target_id)) {
        throw std::runtime_error(
            fmt::format("Duplicated target id: {:#x}\n", target_id));
      }
      brawl_music_id_to_unique_ids_map[target_id] = unique_music_ids;
    });

    std::osyncstream(std::cout) << fmt::format(
        "[+] Loaded playlist {} successfully!\n", entry.first.str());
  }
  m_brawl_music_id_to_unique_ids_map = brawl_music_id_to_unique_ids_map;

  // for debugging
  /*
  for (auto [k, v] : m_brawl_music_id_to_unique_ids_map) {
    for (auto id : v) {
      std::osyncstream(std::cout) << fmt::format(
          "brawl music id: {:#x}, unique music id: {:#x}\n", k, id);
    }
  }
  */

  this->init_mt();
}

std::optional<music_entry>
Playlist::random_music_for(std::uint16_t const music_id) {
  if (!m_brawl_music_id_to_unique_ids_map.count(music_id)) {
    return std::nullopt;
  }

  auto const &unique_music_ids = m_brawl_music_id_to_unique_ids_map[music_id];

  if (unique_music_ids.size() == 0) {
    return std::nullopt;
  }
  // choose one randomly

  std::uniform_int_distribution<> dist(0, unique_music_ids.size() - 1);

  auto const unique_music_id = unique_music_ids[dist(m_mt)];

  if (!m_music_map.count(unique_music_id)) {
    return std::nullopt;
  }

  auto entry = m_music_map[unique_music_id];
  return entry;

  // entries
}

void Playlist::init_mt() {
  // seed mt with current time
  std::time_t t = std::time(0); // get time now
  std::tm *now = std::gmtime(&t);
  std::vector<int> buf{};
  buf.push_back(now->tm_year);
  buf.push_back(now->tm_mon);
  buf.push_back(now->tm_mday);
  buf.push_back(now->tm_hour);

  auto seed = std::seed_seq(buf.begin(), buf.end());
  std::mt19937 mt(seed);

  m_mt = mt;
}

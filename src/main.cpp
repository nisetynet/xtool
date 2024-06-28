#include "dolphin_manager.hpp"
#include "playlist.hpp"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <argparse/argparse.hpp>
#include <atomic>
#include <cassert>
#include <constants.hpp>
#include <inspection.hpp>
#include <iostream>
#include <music_player.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <test_seed.hpp>
#include <thread>
#include <unordered_set>
#include <xtool.hpp>

#ifdef _WIN32
#include <winsock.h>
#else
#include <endian.h>
#endif

std::atomic_uint16_t CURRENT_MUSIC_ID(0xffff); // 0xffff = no music
std::atomic_uint32_t CURRENT_G_MTRAND_SEED(0x0);

static const std::unordered_set<std::uint16_t> IGNORE_MUSIC_ID_SET{0xffff,
                                                                   0xcccc, 0x0};
/*
TODO: delete
void print_dolphin_status(DolphinComm::DolphinAccessor const &dolphin) {
  auto const status = dolphin.getStatus();
  std::string s{};
  if (status == DolphinComm::DolphinStatus::hooked) {
    s = "hooked";
  } else if (status == DolphinComm::DolphinStatus::noEmu) {
    s = "process found, not in game";
  } else if (status == DolphinComm::DolphinStatus::notRunning) {
    s = "dolphin process not found";
  } else if (status == DolphinComm::DolphinStatus::unHooked) {
    s = "un hooked";
  }

  spdlog::info("dolphin status: {}", s);
}
*/

void music_player_thread_main(Playlist &&playlist,
                              bool const is_use_std_random_device) {
  try {
    // initialize system

    spdlog::info("Initialize music player.");
    auto music_player = MusicPlayer();
    spdlog::info("Initialized music player successfully.");

    std::uint16_t current_music_id{0xffff};
    std::optional<MusicEntry> current_music_entry = std::nullopt;

    while (true) {
      std::uint16_t const music_id =
#ifdef _WIN32
          // use winsock function
          ntohs(CURRENT_MUSIC_ID.load());
#else
          be16toh(CURRENT_MUSIC_ID.load());
#endif

      if (music_id != current_music_id) {
        spdlog::info("Music change detected: {:#x} -> {:#x}", current_music_id,
                     music_id);
        current_music_id = music_id;

        if (IGNORE_MUSIC_ID_SET.contains(current_music_id)) {
          spdlog::info("Ignore music id {:#x}", current_music_id);
          continue;
        }

        // music changed in game, play

        // retrieve g_mtRand.seed value
        std::uint32_t const seed = CURRENT_G_MTRAND_SEED.load();
        std::optional<MusicEntry> music_entry_opt;
        if (is_use_std_random_device) {
          music_entry_opt =
              playlist.random_music_for_with_std_random_device(music_id);
        } else {
          music_entry_opt =
              playlist.random_music_for_with_g_mtRand_seed(music_id, seed);
        }

        if (!music_entry_opt.has_value()) {
          spdlog::warn("No music entry found for music id {:#x}.", music_id);
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }

        auto const music_entry = music_entry_opt.value();
        current_music_entry = music_entry;

        if (!music_player.play(music_entry)) {
          spdlog::error("Failed to play music.");
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          continue;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } catch (std::exception const &e) {
    spdlog::error("Exception: {}", e.what());
  }
}

void xtool_play_music_main(std::string_view const config_file_path,
                           bool const is_use_std_random_device) {
  DolphinManager dm;

  spdlog::info("Load config file '{}'.", config_file_path);
  Playlist pl(config_file_path);
  spdlog::info("Loaded config file successfully.");

  auto music_player_thread = std::thread(
      music_player_thread_main, std::move(pl), is_use_std_random_device);

  while (true) {

    // read emulator memory
    std::uint16_t music_id;
    // assert(dolphin.isValidConsoleAddress(CURRENT_MUSIC_ID_ADDRESS));
    auto const read1 = dm.dolphin().readFromRAM(
        xtool::constants::CURRENT_MUSIC_ID_ADDRESS, (char *)(&music_id),
        sizeof(std::uint16_t), false);

    std::uint32_t seed;
    static_assert(sizeof(std::uint32_t) == 0x4);
    auto const read2 = dm.dolphin().readFromRAM(
        xtool::constants::G_MTRAND_SEED_ADDRESS, (char *)(&seed), 0x4, false);

    if (!read1) {
      spdlog::error("Failed to read current music id from the game memory.");

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    if (!read2) {
      spdlog::error("Failed to read g_mtRand.seed from the game memory.");

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    // spdlog::info("Current music id:
    //  {:#x}", music_id);

    CURRENT_MUSIC_ID.store(music_id);
    CURRENT_G_MTRAND_SEED.store(seed);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // TODO: maybe handle signals?
  music_player_thread.join();
}

int main(int argc, char **argv) {

  argparse::ArgumentParser program("xtool");
  program.add_argument("--config")
      .help("xtool config toml file path to use.")
      .default_value(std::string("./config.toml"));
  program.add_argument("--use-random-device")
      .help("Use std::random_device instead of in game g_mtRand.seed for "
            "random function seeds.")
      .flag();

  argparse::ArgumentParser sub_command_inspect_config("inspect-config");
  sub_command_inspect_config.add_description(
      "Inspect xtool config and playlists.");
  sub_command_inspect_config.add_argument("--config")
      .help("xtool config toml file path to use.")
      .default_value(std::string("./config.toml"));

  argparse::ArgumentParser sub_command_inspect_musics("inspect-musics");
  sub_command_inspect_musics.add_description("Inspect registered musics.");
  sub_command_inspect_musics.add_argument("--config")
      .help("xtool config toml file path to use.")
      .default_value(std::string("./config.toml"));
  sub_command_inspect_musics.add_argument("--ids")
      .nargs(argparse::nargs_pattern::at_least_one)
      .scan<'u', std::uint64_t>()
      .help("Unique music ids to inspect.")
      .required();

  argparse::ArgumentParser sub_command_seedtest("seedtest");
  sub_command_seedtest.add_description(
      "Test random function with g_mtRand.seed.");
  sub_command_seedtest.add_argument("random distribution range start")
      .scan<'u', std::uint32_t>()
      .default_value(std::uint32_t{0})
      .help("");
  sub_command_seedtest.add_argument("random distribution range end")
      .scan<'u', std::uint32_t>()
      .default_value(std::uint32_t(300))
      .help("");
  sub_command_seedtest.add_argument("count")
      .scan<'u', std::uint32_t>()
      .default_value(std::uint32_t{3000})
      .help("");

  program.add_subparser(sub_command_inspect_config);
  program.add_subparser(sub_command_inspect_musics);
  program.add_subparser(sub_command_seedtest);

  try {
    program.parse_args(argc, argv);

    auto logger = spdlog::stdout_color_mt("xtool-logger");
    spdlog::set_default_logger(logger);
#ifdef XTOOL_DEBUG
    spdlog::set_level(spdlog::level::debug);
#endif
    spdlog::set_pattern("[%Y-%m-%d %T.%f] [%^%l%$] [thread %t] %v");

    if (program["help"] == true) {
      std::cout << program << std::endl;
      return EXIT_SUCCESS;
    }
    spdlog::info("xtool launched");

    if (program.is_subcommand_used(sub_command_inspect_config)) {
      auto const config_path =
          sub_command_inspect_config.get<std::string>("--config");
      inspect_config(config_path);
      return EXIT_SUCCESS;
    }

    if (program.is_subcommand_used(sub_command_inspect_musics)) {
      auto const config_path =
          sub_command_inspect_musics.get<std::string>("--config");
      auto const unique_music_ids =
          sub_command_inspect_musics.get<std::vector<std::uint64_t>>("--ids");
      inspect_musics(config_path, unique_music_ids);
      return EXIT_SUCCESS;
    }

    if (program.is_subcommand_used(sub_command_seedtest)) {
      auto const start = sub_command_seedtest.get<std::uint32_t>(
          "random distribution range start");
      auto const end = sub_command_seedtest.get<std::uint32_t>(
          "random distribution range end");
      auto const count = sub_command_seedtest.get<std::uint32_t>("count");
      test_seed(start, end, count);
      return EXIT_SUCCESS;
    }

    auto const config_file_path = program.get<std::string>("--config");
    auto const is_use_std_random_device =
        program.get<bool>("--use-random-device");
    xtool_play_music_main(config_file_path, is_use_std_random_device);

    // try to find dolphin process

    spdlog::info("xtool exit.");
    return EXIT_SUCCESS;
  } catch (std::exception const &e) {
    spdlog::error("Exception: {}", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_FAILURE;
}

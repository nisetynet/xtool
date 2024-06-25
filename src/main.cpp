#include "playlist.hpp"
#include <cstdint>
#include <cstdlib>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <args.hpp>
#include <atomic>
#include <cassert>
#include <inspection.hpp>
#include <iostream>
#include <music_player.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <syncstream>
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

auto static constexpr CURRENT_MUSIC_ID_ADDRESS = 0x90e60f06 - 0x80000000;
auto static constexpr G_MTRAND_SEED_ADDRESS = 0x805a00b8 + 0x4 - 0x80000000;
static const std::unordered_set<std::uint16_t> IGNORE_MUSIC_ID_SET{0xffff,
                                                                   0xcccc, 0x0};

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

void music_player_thread_main(Playlist &&playlist) {
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

        // retrive g_mtRand.seed value
        std::uint32_t const seed = CURRENT_G_MTRAND_SEED.load();

        auto const music_entry_opt = playlist.random_music_for(music_id, seed);
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

int main(int argc, char **argv) {

  try {
    auto sleep_fn = [](int const msec) {
      std::this_thread::sleep_for(
          std::chrono::duration(std::chrono::milliseconds(msec)));
    };

    auto logger = spdlog::stdout_color_mt("xtool-logger");
    spdlog::set_default_logger(logger);
    // spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %T.%f] [%^%l%$] [thread %t] %v");

    auto const vm = parse_program_options(argc, argv);
    auto const config_file_path =
        std::filesystem::path(vm["config"].as<std::string>());

    spdlog::info("xtool launched");

    spdlog::info("Load config file '{}'", config_file_path.string());
    Playlist pl(config_file_path);
    spdlog::info("Loaded config file successfully.");

    if (vm.count("inspect")) {
      inspect_config(pl);
      return EXIT_SUCCESS;
    }

    auto dolphin = DolphinComm::DolphinAccessor{};
    dolphin.init();

    // try to find dolphin process
    while (true) {
      spdlog::info("Waiting for dolphin ...");

      dolphin.hook();
      auto const status = dolphin.getStatus();

      print_dolphin_status(dolphin);

      if (status == DolphinComm::DolphinStatus::hooked) {
        break;
      }

      sleep_fn(100);
    }

    spdlog::info("dolphin process hooked! (pid={:#x})", dolphin.getPID());

    auto music_player_thread =
        std::thread(music_player_thread_main, std::move(pl));

    while (true) {
      auto const is_game_running =
          (dolphin.getStatus() == DolphinComm::DolphinStatus::hooked);
      if (is_game_running) {
        // read emulator memory
        std::uint16_t music_id;
        // assert(dolphin.isValidConsoleAddress(CURRENT_MUSIC_ID_ADDRESS));
        auto const read1 =
            dolphin.readFromRAM(CURRENT_MUSIC_ID_ADDRESS, (char *)(&music_id),
                                sizeof(std::uint16_t), false);

        std::uint32_t seed;
        static_assert(sizeof(std::uint32_t) == 0x4);
        auto const read2 = dolphin.readFromRAM(G_MTRAND_SEED_ADDRESS,
                                               (char *)(&seed), 0x4, false);

        if (!read1) {
          spdlog::error("Failed to read current music id.");

          sleep_fn(10);
          continue;
        }
        if (!read2) {
          spdlog::error("Failed to read g_mtRand.seed.");
          sleep_fn(10);
          continue;
        }

        // spdlog::info("Current music id:
        //  {:#x}", music_id);

        CURRENT_MUSIC_ID.store(music_id);
        CURRENT_G_MTRAND_SEED.store(seed);
        sleep_fn(200);
        continue;
      }

      spdlog::warn("Game is not running.");
    }
    music_player_thread.join();
    spdlog::info("xtool exit.");
    return EXIT_SUCCESS;
  } catch (boost::program_options::error &e) {
    spdlog::error("Invalid argument: {}", e.what());
    return EXIT_FAILURE;
  } catch (std::exception const &e) {
    spdlog::error("Exception: {}", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_FAILURE;
}

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <atomic>
#include <cassert>
#include <iostream>
#include <music_player.hpp>
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

auto static constexpr CURRENT_MUSIC_ID_ADDRESS = 0x90e60f06 - 0x80000000;

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

void music_player_thread_main() {
  try {
    auto playlist = Playlist("./config.toml");
    // initialize system
    spdlog::info("Loading playlist from config file...");

    auto music_player = MusicPlayer();
    spdlog::info("Loaded playlist successfully!");

    std::uint16_t current_music_id{0xffff};
    std::optional<music_entry> current_music_entry = std::nullopt;
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

        auto const music_entry_opt = playlist.random_music_for(music_id);
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

int main() {

  auto sleep_fn = [](int const msec) {
    std::this_thread::sleep_for(
        std::chrono::duration(std::chrono::milliseconds(msec)));
  };
  spdlog::info("xtool launched.");

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

  auto music_player_thread = std::thread(music_player_thread_main);

  while (true) {
    auto const is_game_running =
        (dolphin.getStatus() == DolphinComm::DolphinStatus::hooked);
    if (is_game_running) {
      // read emulator memory
      std::uint16_t music_id;
      // assert(dolphin.isValidConsoleAddress(CURRENT_MUSIC_ID_ADDRESS));
      auto const read =
          dolphin.readFromRAM(CURRENT_MUSIC_ID_ADDRESS, (char *)(&music_id),
                              sizeof(std::uint16_t), false);
      if (!read) {
        spdlog::error("Failed to read current music id.");

        sleep_fn(10);
        continue;
      }
      // spdlog::info("Current music id:
      //  {:#x}", music_id);

      CURRENT_MUSIC_ID.store(music_id);
      sleep_fn(200);
      continue;
    }

    spdlog::warn("Game is not running.");
  }
  music_player_thread.join();
  spdlog::info("xtool exit.");
  return EXIT_SUCCESS;
}

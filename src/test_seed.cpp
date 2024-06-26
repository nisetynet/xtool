#include <chrono>
#include <constants.hpp>
#include <cstdint>
#include <dolphin_manager.hpp>
#include <playlist.hpp>
#include <spdlog/spdlog.h>
#include <test_seed.hpp>
#include <thread>

void test_seed(std::uint32_t const dist_l, std::uint32_t const dist_r,
               std::uint32_t const try_count) {
  DolphinManager dm;
  spdlog::info("start test_seed");

  std::map<std::size_t, std::uint32_t> occurences;

  std::uint32_t prev_seed = -1;
  auto count = try_count;
  for (;;) {
    if (count == 0) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::uint32_t seed;
    if (!dm.dolphin().readFromRAM(xtool::constants::G_MTRAND_SEED_ADDRESS,
                                  (char *)&seed, 0x4, false)) {
      spdlog::error("Failed to read dolphin memory.");
    }
    if (seed == prev_seed) {
      continue;
    }
    prev_seed = seed;
    auto const rand = seed_rand(dist_l, dist_r, seed);
    spdlog::info("rand={}, remain={}", rand, count);
    if (auto [ignore_, inserted] = occurences.insert(std::make_pair(rand, 1));
        !inserted) {
      // update
      ++occurences[rand];
    }
    --count;
  }

  spdlog::info("rand dist range={}~{}, seeded {} times.", dist_l, dist_r,
               try_count);
  for (auto [rand, occurences] : occurences) {
    spdlog::info("value={}, count={}", rand, occurences);
  }
  spdlog::info("Total {} unique rand", occurences.size());
  spdlog::info("test_seed end");
}
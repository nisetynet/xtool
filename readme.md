## xtool
Super Smash Bros. Brawl(SSBB, Brawl) music player for [dolphin emulator](https://dolphin-emu.org/).  
Works with RSBE and RSBJ.  
Works on Linux and Windows.

## Motive
https://nisety.net/posts/2023/12/27/dolphin-emulator-brawl-custom-musics/#%E5%8B%95%E6%A9%9F

## Demo
https://www.youtube.com/watch?v=XIIiO7Qqtvs

## Dependencies
* py-dolphin-memory-engine: https://github.com/henriquegemignani/py-dolphin-memory-engine  
Modified and directly included in the source.
* C++ fmt Library: https://github.com/fmtlib/fmt
* miniaudio: https://github.com/mackron/miniaudio#license
* toml++: https://marzer.github.io/tomlplusplus/
## How to build
You need a reasonably recent C++ compiler(C++20) and [meson](https://mesonbuild.com/Getting-meson.html) to build.


### Common
Install meson.

Clone the xtool git repository.

```bash
git clone https://github.com/nisetynet/xtool
```

#### Linux
If you are using linux, you know what to do.

In the source root directory, run  
```bash
bash build.sh
```

You can find a built xtool binary in the `build` directory.

#### Windows
Ensure you have installed latest MSVC compiler.  

You can use VisualStudio Installer to install latest MSVC: <https://learn.microsoft.com/en-us/visualstudio/install/install-visual-studio?view=vs-2022>

In the source root directory, run  
```bash
./build.bat
```

You can find a built xtool binary in the `win_build` directory.


## Usage
TODO (Put music files, write config file, launch xtool.)


## ⚠️
Some features are broken.  
See https://nisety.net/posts/2023/12/27/dolphin-emulator-brawl-custom-musics/#%E5%95%8F%E9%A1%8C%E7%82%B9%E7%AD%89 for details.

## Changelog
* 2024-06-25  
    * Better rand seed(reads `g_mtRand.seed`).
    * Replace std::osyncstream(std::cout) with spdlog.
    * Added meson build file.
    * Trivial refactor.
    * Better log format.
    * Added Playlist inspection command.(`xtool --inspect`)
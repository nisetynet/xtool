## xtool
Super Smash Bros. Brawl(SSBB, Brawl) music player for [dolphin emulator](https://dolphin-emu.org/).  
Works with RSBE and RSBJ.  
Works on Linux and Windows.

## Motive
https://nisety.net/posts/2023/12/27/dolphin-emulator-brawl-custom-musics/#%E5%8B%95%E6%A9%9F

## Demo
https://www.youtube.com/watch?v=XIIiO7Qqtvs

## Dependencies
* Python Dolphin Memory Engine: https://github.com/henriquegemignani/py-dolphin-memory-engine
* C++ fmt Library: https://github.com/fmtlib/fmt
* miniaudio: https://github.com/mackron/miniaudio#license

## How to build
You need a C++ compiler and [cmake](https://cmake.org/download/) to build.

### Linux
In the source root directory, run
```bash
mkdir build && cd build && cmake .. && make -j$(nproc)
```

### Windows
In the source root directory, run
```bash
cmake ..
```
And build the generated visual studio project.

## Usage
TODO

## ⚠️
Some features are broken.  
See https://nisety.net/posts/2023/12/27/dolphin-emulator-brawl-custom-musics/#%E5%95%8F%E9%A1%8C%E7%82%B9%E7%AD%89 for details.

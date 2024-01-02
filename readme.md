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
You need a C++ compiler and [cmake](https://cmake.org/download/) to build.

### Linux
In the source root directory, run
```bash
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
```
Replace `clang` with `g++` to build with g++.

### Windows
In the source root directory, run
```bash
cmake -B build -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
```

## Usage
TODO

## ⚠️
Some features are broken.  
See https://nisety.net/posts/2023/12/27/dolphin-emulator-brawl-custom-musics/#%E5%95%8F%E9%A1%8C%E7%82%B9%E7%AD%89 for details.

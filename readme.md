## xtool
Super Smash Bros. Brawl(SSBB, Brawl) music player for dolphin.  
Works with RSBE and RSBJ.  
Works on Linux and Windows.

## Demo
https://www.youtube.com/watch?v=XIIiO7Qqtvs

## Dependencies(included in the source directly)
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

## ⚠️
Some features are broken.  
See https://nisety.net/posts/2023/12/27/dolphin-emulator-brawl-custom-musics/#%E5%95%8F%E9%A1%8C%E7%82%B9%E7%AD%89 for details.

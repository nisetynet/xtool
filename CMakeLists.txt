cmake_minimum_required(VERSION 3.25.1)
project(xtool CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(fmt)
include_directories(fmt/include)
include_directories(include)

add_executable(xtool src/main.cpp src/playlist.cpp src/music_player.cpp include/dme/DolphinProcess/Linux/LinuxDolphinProcess.cpp
include/dme/DolphinProcess/Windows/WindowsDolphinProcess.cpp
include/dme/DolphinProcess/DolphinAccessor.cpp
include/dme/Common/MemoryCommon.cpp

)

if (WIN32)
target_link_libraries(xtool PRIVATE fmt::fmt-header-only wsock32)
else()
# find pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# miniaudio doc: The Linux build only requires linking to -ldl, -lpthread and -lm. You do not need any development packages. You may need to link with -latomic if you're compiling for 32-bit ARM. 
target_link_libraries(xtool PRIVATE fmt::fmt-header-only dl Threads::Threads m)
endif()
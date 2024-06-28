#pragma once
#include <filesystem>
#include <playlist.hpp>

void inspect_config(std::string_view const config_file_path);

void inspect_musics(std::string_view const config_file_path,
                    std::vector<UniqueMusicID> const &unique_music_ids);
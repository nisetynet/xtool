meson setup --wipe -Dbuildtype=release -Ddefault_library=static win_build
cd win_build
meson compile

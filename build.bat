meson setup --wipe -Dbuildtype=release -Dcpp_std=vc++latest -Ddefault_library=static win_build
cd win_build
meson compile

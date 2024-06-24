set -eux

meson setup --wipe -Dbuildtype=release -Ddefault_library=static build && cd build
meson compile

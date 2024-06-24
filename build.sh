set -eux

meson setup --wipe -Dbuildtype=debug -Ddefault_library=static build && cd build
meson compile

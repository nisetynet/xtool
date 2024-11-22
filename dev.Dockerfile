FROM ubuntu:latest

# g++-mingw-w64
RUN apt-get update && apt-get install -y git gdb python3 python3-pip python3-setuptools python3-wheel ninja-build lsb-release wget cmake software-properties-common gnupg pkg-config libssl-dev build-essential clang clangd clang-tools
# install latest meson
RUN python3 -m pip install --break-system-packages meson

# install clang, clangd
# https://apt.llvm.org/
#RUN wget https://apt.llvm.org/llvm.sh \
#&& chmod +x llvm.sh  \
#&& apt-get update \
#&& ./llvm.sh 18


# RUN apt-get clean && rm -rf /var/lib/apt/lists/*

CMD ["sleep","infinity"]
FROM ubuntu:latest

# g++-mingw-w64
RUN apt-get update && apt-get install -y git gdb python3 python3-pip python3-setuptools python3-wheel ninja-build lsb-release wget cmake software-properties-common gnupg pkg-config libssl-dev build-essential clang clangd clang-tools
# install latest meson
RUN python3 -m pip install --break-system-packages meson
# download & install boost
# https://stackoverflow.com/questions/12578499/how-to-install-boost-on-ubuntu
# remote
RUN wget -O boost_1_85_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.85.0/boost_1_85_0.tar.gz/download \
&& tar xzvf boost_1_85_0.tar.gz
RUN cd boost_1_85_0 && ./bootstrap.sh
RUN cd boost_1_85_0 && ./b2 install
# RUN cd boost_1_85_0 && ./b2 toolset=gcc-mingw target-os=windows install


# github version(not tested
#RUN  wget https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.gz \
#&& tar xzvf boost-1.85.0-cmake.tar.gz \
#&& cd boost-1.85.0-cmake && mkdir __build \ 
#&& cd __build \
#&& cmake .. \
#&& cmake --build . --target install --config Release


# install clang, clangd
# https://apt.llvm.org/
#RUN wget https://apt.llvm.org/llvm.sh \
#&& chmod +x llvm.sh  \
#&& apt-get update \
#&& ./llvm.sh 18


# RUN apt-get clean && rm -rf /var/lib/apt/lists/*

CMD ["sleep","infinity"]
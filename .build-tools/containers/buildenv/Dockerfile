FROM ubuntu:24.04

ENV USER=hicr
ENV ROOT_USER=root
ENV HOME /home/$USER

RUN apt update --fix-missing
RUN DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends tzdata

RUN apt install -y bash
SHELL ["/bin/bash", "-c"]


RUN apt install -y build-essential
RUN apt install -y git
RUN apt install -y libgtest-dev
RUN apt install -y libhwloc-dev
RUN apt install -y openssh-client
RUN apt install -y libopenblas-dev
RUN apt install -y liblapack-dev
RUN apt install -y liblapacke-dev
RUN apt install -y libfabric-dev
RUN apt install -y libibverbs-dev
RUN apt install -y infiniband-diags
RUN apt install -y libboost-context-dev
RUN apt install -y curl
RUN apt install -y jq 
RUN apt install -y python3-pip
RUN apt install -y python3-venv
RUN apt install -y pkgconf
RUN apt install -y wget 
RUN apt install -y sudo
RUN apt install -y openssh-client 

# keep it to make openmpi compile
RUN apt install -y g++-10
RUN apt install -y gcc-10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 20
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 20

RUN useradd -d $HOME -m $USER -G root \
  && chown -R $USER /home/$USER  \
  && echo 'hicr ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers \
  && chmod 750 $HOME \
  && echo 'umask 0027' >> $HOME/.bashrc \
  && adduser $USER sudo

USER $USER

RUN python3 -m venv ${HOME}/.venv
RUN source ${HOME}/.venv/bin/activate \
  && pip3 install --upgrade pip \
  && pip3 install 'cmake>=3.31.4' \
  && pip3 install 'meson>=1.2' \
  && pip3 install 'ninja>=1.10' \
  && pip3 install 'gcovr>=6.0'

USER ${ROOT_USER}

ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib/
ENV PATH=${PATH}:$HOME/.venv/bin

RUN wget https://github.com/openucx/ucx/releases/download/v1.15.0/ucx-1.15.0.tar.gz \
  && tar -zxf ucx-1.15.0.tar.gz \
  && cd ucx-1.15.0 \ 
  && mkdir build \
  && cd build \
  && ../configure --prefix=/usr/local \
  && make -j30 \
  && sudo make install \
  && rm -rf /ucx-1.15.0.tar.gz \
  && rm -rf /ucx-1.15.0

RUN wget https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.2.tar.gz \
  && tar -zxf openmpi-5.0.2.tar.gz \
  && cd openmpi-5.0.2 \ 
  && mkdir build \
  && cd build \
  && ../configure --with-ucx=/usr/local --prefix=/usr/local \
  && make -j30 \
  && sudo make install \
  && rm -rf /openmpi-5.0.2.tar.gz \
  && rm -rf /openmpi-5.0.2

USER $USER

# Download lpf
RUN git clone -b noc_extension --single-branch --depth 1 https://github.com/Algebraic-Programming/LPF.git $HOME/lpf \
  && cd $HOME/lpf \
  && git checkout noc_extension \
  && mkdir ./build \
  && cd ./build \
  && ../bootstrap.sh --prefix=/usr/local \
  && make -j24 \
  && sudo make install || true \
  && sudo rm -rf $HOME/lpf


RUN git clone -b 1.11.0 --single-branch --depth 1 https://github.com/bsc-pm/ovni.git $HOME/ovni \
  && cd $HOME/ovni  \
  && mkdir build  \
  && cd build \ 
  && cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=FALSE  \
  && make -j24  \
  && sudo make install  \
  && sudo rm -rf $HOME/ovni

# Download nosv
RUN git clone -b 3.1.0 --single-branch --depth 1 https://github.com/bsc-pm/nos-v.git $HOME/nos-v && \
  cd $HOME/nos-v && \
  autoreconf -f -i -v && \
  ./configure --prefix=/usr/local --with-ovni=/usr && \
  make all && \
  sudo make install && \
  cd .. && \
  rm -rf $HOME/nos-v && \
  sudo sed -i '/^\[instrumentation\]/,/^\[/{s/version *= *"none"/version = "ovni"/}' /usr/local/share/nosv.toml && \
  sudo sed -i '/^\[ovni\]/,/^\[/{s/level *= *2/level = 0/}' /usr/local/share/nosv.toml

# Download opencl
RUN sudo apt install -y cmake libstb-dev libsfml-dev libglew-dev libglm-dev libtclap-dev ruby doxygen -y
RUN git clone -b v2024.10.24 --single-branch --depth 1 https://github.com/KhronosGroup/OpenCL-SDK.git $HOME/opencl && \
  cd $HOME/opencl && \
  git submodule update --init --recursive && \
  cmake -D CMAKE_INSTALL_PREFIX=/usr/local -B ./build -S . && \
  sudo cmake --build ./build --config Release --target install && \
  cd .. && \
  rm -rf $HOME/opencl

# Download protobuf
RUN git clone --recursive -b v29.3 --single-branch --depth 1 https://github.com/protocolbuffers/protobuf.git $HOME/protobuf && \
  cd $HOME/protobuf && \
  git submodule update --init --recursive && \
  mkdir build && \
  cd    build && \
  cmake -D CMAKE_INSTALL_PREFIX=/usr/local  \
  -D CMAKE_BUILD_TYPE=Release  \
  -D CMAKE_SKIP_INSTALL_RPATH=ON  \
  -D protobuf_BUILD_TESTS=OFF              \
  -D protobuf_ABSL_PROVIDER=module        \
  -D protobuf_BUILD_LIBUPB=OFF             \
  -D protobuf_BUILD_SHARED_LIBS=ON         \
  -G Ninja .. &&  \
  sudo ${HOME}/.venv/bin/ninja && \
  sudo ${HOME}/.venv/bin/ninja install && \
  cd .. && \
  rm -rf $HOME/protobuf
  

RUN sudo apt install -y intel-opencl-icd
RUN sudo apt install -y nvidia-driver-570-server

RUN sudo apt purge -y g++-10
RUN sudo apt purge -y gcc-10
RUN sudo apt install -y g++-14
RUN sudo apt install -y gcc-14
RUN sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 30
RUN sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 30
RUN sudo update-alternatives --install /usr/bin/gcov gcov /usr/bin/gcov-14 30

RUN touch $HOME/.hicr-env.sh
RUN chmod +rwx $HOME/.hicr-env.sh
RUN echo 'export PATH=$HOME/.venv/bin:$PATH' >> $HOME/.hicr-env.sh
RUN echo 'export PATH=/usr/local/bin:$PATH' >> $HOME/.hicr-env.sh
RUN echo 'export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH' >> $HOME/.hicr-env.sh
RUN echo 'export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH' >> $HOME/.hicr-env.sh
RUN echo 'export LIBRARY_PATH=/usr/local/lib:$LIBRARY_PATH' >> $HOME/.hicr-env.sh
RUN echo 'export CPATH=/usr/local/include:$CPATH' >> $HOME/.hicr-env.sh
RUN echo 'export OMPI_ALLOW_RUN_AS_ROOT=1' >> $HOME/.hicr-env.sh
RUN echo 'export OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1' >> $HOME/.hicr-env.sh
RUN echo 'export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/lib/x86_64-linux-gnu/pkgconfig' >> $HOME/.hicr-env.sh
RUN echo 'export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/lib/pkgconfig' >> $HOME/.hicr-env.sh
RUN echo 'export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig' >> $HOME/.hicr-env.sh
RUN echo 'export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/share/pkgconfig' >> $HOME/.hicr-env.sh
RUN echo 'source ${HOME}/.venv/bin/activate' >> $HOME/.hicr-env.sh


RUN echo 'source $HOME/.hicr-env.sh' >> $HOME/.bashrc
USER $ROOT_USER
RUN chown -R $USER $HOME
RUN chgrp -R $USER $HOME

WORKDIR $HOME

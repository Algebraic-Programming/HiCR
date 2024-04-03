FROM ubuntu:20.04

ENV USER hicr
ENV ROOT_USER root
ENV HOME /home/$USER

RUN apt update --fix-missing
RUN DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends tzdata

RUN apt install -y build-essential
RUN apt install -y git
RUN apt install -y bash
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
RUN apt install -y pkgconf
RUN apt install -y wget 
RUN apt install -y sudo
RUN apt install -y cmake

RUN apt install -y g++-10
RUN apt install -y gcc-10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 20
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 20

ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib

RUN wget https://github.com/openucx/ucx/releases/download/v1.15.0/ucx-1.15.0.tar.gz \
  && tar -zxf ucx-1.15.0.tar.gz \
  && cd ucx-1.15.0 \ 
  && mkdir build \
  && cd build \
  && ../configure --prefix=/usr/local \
  && make -j30 \
  && make install \
  && rm -rf /ucx-1.15.0.tar.gz \
  && rm -rf /ucx-1.15.0

RUN wget https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.2.tar.gz \
  && tar -zxf openmpi-5.0.2.tar.gz \
  && cd openmpi-5.0.2 \ 
  && mkdir build \
  && cd build \
  && ../configure --with-ucx=/usr/local --prefix=/usr/local \
  && make -j30 \
  && make install \
  && rm -rf /openmpi-5.0.2.tar.gz \
  && rm -rf /openmpi-5.0.2

RUN useradd -d $HOME -m $USER -G root \
  && chown -R $USER /home/$USER  \
  && echo 'hicr ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers \
  && chmod 750 $HOME \
  && echo 'umask 0027' >> $HOME/.bashrc \
  && adduser $USER sudo

USER $USER

RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install 'cmake>=3.27'
RUN python3 -m pip install 'meson>=1.2'
RUN python3 -m pip install 'ninja>=1.10'
RUN python3 -m pip install 'gcovr>=6.0'

RUN mkdir $HOME/lpf

RUN touch $HOME/.hicr-env.sh
RUN chmod +rwx $HOME/.hicr-env.sh
RUN echo 'export PATH=$HOME/.local/bin:$PATH' >> $HOME/.hicr-env.sh
RUN echo 'export PATH=/home/hicr/lpf/bin:$PATH' >> $HOME/.hicr-env.sh
RUN echo 'export LD_LIBRARY_PATH=/usr/local/lib:$PATH' >> $HOME/.hicr-env.sh
RUN echo 'export LD_LIBRARY_PATH=/home/hicr/lpf/lib:$LD_LIBRARY_PATH' >> $HOME/.hicr-env.sh
RUN echo 'export LIBRARY_PATH=/home/hicr/lpf/lib:$LIBRARY_PATH' >> $HOME/.hicr-env.sh
RUN echo 'export CPATH=/home/hicr/lpf/include:$CPATH' >> $HOME/.hicr-env.sh
RUN echo 'export OMPI_ALLOW_RUN_AS_ROOT=1' >> $HOME/.hicr-env.sh
RUN echo 'export OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1' >> $HOME/.hicr-env.sh

RUN echo 'source $HOME/.hicr-env.sh' >> $HOME/.bashrc
RUN chown -R $USER $HOME
RUN chgrp -R $USER $HOME

USER $ROOT_USER
WORKDIR $HOME

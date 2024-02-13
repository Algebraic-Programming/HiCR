FROM ubuntu:20.04

ENV USER hicr
ENV ROOT_USER root
ENV HOME /home/$USER

RUN apt update
RUN DEBIAN_FRONTEND=noninteractive apt install -y --no-install-recommends tzdata

RUN apt install -y build-essential
RUN apt install -y git
RUN apt install -y bash
RUN apt install -y libgtest-dev
RUN apt install -y libboost-all-dev
RUN apt install -y libhwloc-dev
RUN apt install -y openssh-client
RUN apt install -y libopenblas-dev
RUN apt install -y liblapack-dev
RUN apt install -y liblapacke-dev
RUN apt install -y curl
RUN apt install -y jq 
RUN apt install -y libopenmpi-dev
RUN apt install -y python3-pip
RUN apt install -y pkgconf
RUN apt install -y wget 
RUN apt install -y sudo

RUN apt install -y g++-10
RUN apt install -y gcc-10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 20
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 20

RUN sudo python3 -m pip install 'cmake>=3.27'
RUN sudo python3 -m pip install 'meson>=1.2'
RUN sudo python3 -m pip install 'ninja>=1.10'
RUN sudo python3 -m pip install 'gcovr>=6.0'

RUN useradd -d $HOME -m $USER -G root \
  && chown -R $USER /home/$USER  \
  && echo 'hicr ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers \
  && chmod 750 $HOME \
  && echo 'umask 0027' >> $HOME/.bashrc \
  && adduser $USER sudo

USER $USER

RUN touch $HOME/.hicr-env.sh
RUN chmod +rwx $HOME/.hicr-env.sh
RUN echo "export PATH=$HOME/.local/bin:$PATH" >> $HOME/.hicr-env.sh
RUN echo "source .hicr-env.sh" >> $HOME/.bashrc
RUN chown -R $USER $HOME
RUN chgrp -R $USER $HOME

RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install 'cmake>=3.27'
RUN python3 -m pip install 'meson>=1.2'
RUN python3 -m pip install 'ninja>=1.10'
RUN python3 -m pip install 'gcovr>=6.0'

USER $ROOT_USER
WORKDIR /
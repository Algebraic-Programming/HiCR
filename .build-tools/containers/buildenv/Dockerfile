FROM python:3.8

RUN apt update

# Build tools
RUN apt install -y build-essential
RUN apt install -y git
RUN apt install -y bash
RUN apt install -y libgtest-dev
RUN apt install -y libboost-all-dev
RUN apt install -y libhwloc-dev
RUN apt install -y gcovr
RUN apt install -y openssh-client
RUN apt install -y libopenblas-dev
RUN apt install -y cmake
RUN apt install -y liblapack-dev
RUN apt install -y liblapacke-dev
RUN apt install -y curl
RUN apt install -y jq 
RUN apt install -y libopenmpi-dev
RUN apt install -y python3-pip

RUN python3 -m pip install 'meson>=1.2'
RUN python3 -m pip install 'ninja>=1.10'
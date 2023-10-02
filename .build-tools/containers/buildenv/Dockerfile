FROM alpine:3.18.3

RUN apk update

# Build tools
RUN apk add build-base
RUN apk add git
RUN apk add bash
RUN apk add gtest-dev
RUN apk add boost-dev
RUN apk add hwloc-dev
RUN apk add gcovr
RUN apk add openssh-client
RUN apk add openblas-dev
RUN apk add cmake
RUN apk add lapack-dev
RUN apk add curl
RUN apk add jq
RUN apk add openmpi-dev
RUN apk add py3-pip

RUN python3 -m pip install 'meson>=1.2'
RUN python3 -m pip install 'ninja>=1.10'
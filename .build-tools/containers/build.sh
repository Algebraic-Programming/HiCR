#!/usr/bin/env bash

if [[ $# -ne 2 ]]; then
   echo "Usage: $0 <name> <arch>"
   exit 1
fi
folder=${1}
arch=${2}

docker build -t "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/${folder}-${arch}:latest" --build-arg ARCH=${arch} ${folder} 

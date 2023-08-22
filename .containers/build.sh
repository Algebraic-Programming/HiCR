#!/usr/bin/env bash

if [[ $# -ne 1 ]]; then
   echo "Usage: $0 <name>"
   exit 1
fi
folder=${1}

docker build -t "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/${folder}:latest" ${folder}

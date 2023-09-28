#!/usr/bin/env bash

if [[ $# -ne 1 ]]; then
   echo "Usage: $0 <name>"
   exit 1
fi
folder=${1}

docker run -it "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/${folder}:latest" bash 

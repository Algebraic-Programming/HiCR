#!/usr/bin/env bash

if command -v arch &>/dev/null; then
   target_arch=$(arch)

   if [ $target_arch == "aarch64" ]; then
      target_arch="arm64v8"
   else
      target_arch="x86_64"
   fi
else
   if [[ $# -ne 2 ]]; then
      echo "arch not installed. Please provice manually the target architecture. Usage: $0 <name> <arch>"
      exit 1
   else
      arch=${2}
   fi
fi

folder=${1}
echo "Deploying $folder for arch $target_arch"

docker login registry.gitlab.huaweirc.ch
docker push "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/${folder}-${target_arch}:latest" 

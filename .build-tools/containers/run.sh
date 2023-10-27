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
echo "Running $folder for arch $target_arch"

build_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$build_dir/$folder/run.sh ${folder} ${target_arch}

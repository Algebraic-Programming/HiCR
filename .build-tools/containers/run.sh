#!/usr/bin/env bash

if [[ $# -ne 1 ]]; then
   echo "Usage: $0 <name>"
   exit 1
fi
folder=${1}

build_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$build_dir/$folder/run.sh $folder

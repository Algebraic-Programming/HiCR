#!/usr/bin/env bash

if [[ $# -ne 2 ]]; then
 echo "Usage: $0 <name> <arch>"
 exit 1
fi

folder=${1}
arch=${2}

build_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
$build_dir/$folder/run.sh ${folder} ${arch}

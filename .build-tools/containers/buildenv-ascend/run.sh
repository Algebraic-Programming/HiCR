#!/usr/bin/env bash

if [[ $# -ne 2 ]]; then
 echo "Usage: $0 <name> <arch>"
 exit 1
fi

folder=${1}
arch=${2}

docker run --device=/dev/davinci0 --device=/dev/davinci1 --device=/dev/davinci2 --device=/dev/davinci3 --device=/dev/davinci4 --device=/dev/davinci5 --device=/dev/davinci6 --device=/dev/davinci7 --device=/dev/davinci_manager --device=/dev/hisi_hdc --device=/dev/devmm_svm -v /usr/local/dcmi:/usr/local/dcmi -v /usr/local/Ascend/driver:/usr/local/Ascend/driver -v /usr/local/Ascend/firmware:/usr/local/Ascend/firmware -v /usr/local/Ascend/toolbox:/usr/local/Ascend/toolbox -v /usr/local/bin/npu-smi:/usr/local/bin/npu-smi -v /usr/local/bin/npu-smi:/usr/local/bin/npu-smi --name hicr-ascend -t -d registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/${folder}-${arch}:latest bash

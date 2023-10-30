#/usr/bin/env bash

export LD_LIBRARY_PATH=/usr/local/Ascend/ascend-toolkit/latest/fwkacllib/lib64:/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub:$LD_LIBRARY_PATH

meson compile -C build
#!/bin/env bash

atc --singleop=./add.json --soc_version=Ascend910 --output=./ascend
atc --singleop=./relu.json --soc_version=Ascend910 --output=./ascend
atc --singleop=./gemm.json --soc_version=Ascend910 --output=./ascend
#!/bin/env bash

atc --singleop=./add.json --soc_version=Ascend910 --output=./acl
atc --singleop=./relu.json --soc_version=Ascend910 --output=./acl
atc --singleop=./gemm.json --soc_version=Ascend910 --output=./acl
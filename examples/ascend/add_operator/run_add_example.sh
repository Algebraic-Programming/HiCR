#!/bin/bash

# To use this script the operator binary should be already compiled
cp ../../../build/examples/ascend/add_operator/addOperator add_operator_exec 
cd add_operator_exec
./addOperator
rm -rf addOperator

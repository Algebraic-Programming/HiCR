#!/bin/bash

# To use this script the operator binary should be already compiled
cp ../../../build/examples/ascend/add_operator/addOperator . 
./addOperator
rm -rf addOperator

#!/bin/bash

filePath=`realpath ${0}`
dirPath=`dirname ${filePath}`
basePath=`realpath ${dirPath}/..`
echo ${basePath}

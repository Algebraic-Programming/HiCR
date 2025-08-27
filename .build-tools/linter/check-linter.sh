#!/usr/bin/env bash
# Script to check or fix Clang tidy use

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <check|fix> <rootDir>"
    exit 1
fi
task="${1}";
rootDir=`realpath "${2}"`
fileDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
fileList=`find ${rootDir} -name "*.hpp" -not -path "${rootDir}/extern/*" -not -path "${rootDir}/tests/*" -not -path "${rootDir}/examples/*" -not -path "*/channelsImpl.hpp" -not -path "*/acl/*" -not -path "*/exceptions.hpp"`

fixFlag=""
if [[ ${task} == "fix" ]]; then fixFlag="--fix-errors --fix-notes"; fi

for file in ${fileList}; do
 command="clang-tidy ${file} -p ${rootDir}/build ${fixFlag}"  
 echo "Running command: \"${command}\""
 ${command}
 rc=$?
 if [[ ${task} == "check" ]] && [ ${rc} -ne 0 ]; then
  echo "Found error ${rc} in clang-tidy. Please check log above."
  exit 1
 fi
done
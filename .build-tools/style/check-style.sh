#!/usr/bin/env bash
# Script to check or fix Clang format use
# Adapted from https://github.com/cselab/korali/blob/master/tools/style/style_cxx.sh (MIT License ETH Zurich)

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <check|fix> <rootDir>"
    exit 1
fi
task="${1}";
rootDir="${2}"
fileDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

function check()
{
  if [ ! $? -eq 0 ]; then 
    echo "Error fixing style."
    exit -1 
  fi
}

function check_syntax()
{
  echo "Looking into ${rootDir}"
  python3 $fileDir/run-clang-format/run-clang-format.py --recursive ${rootDir} --extensions "hpp,cpp" --exclude "*build/*"
  if [ ! $? -eq 0 ]; then
    echo "Error: C++ Code formatting in file ${d} is not normalized."
    echo "Solution: Please run '$0 fix' to fix it."
    exit -1
  fi
}

function fix_syntax()
{
  basePath=`realpath ${rootDir}` 
  echo "Looking into ${basePath}..."
  src_files=`find ${basePath} \( -name "*.cpp" -o -name "*.hpp" \) -not -path '*/.*' -not -path '*/build/*'`
  
  if [ ! -z "$src_files" ]
  then
    echo $src_files | xargs -n1 echo "Processing: "
    echo $src_files | xargs -n6 -P2 clang-format -style=file -i "$@"
    check
  fi
}

##############################################
### Testing/fixing C++ Code Style
##############################################
command -v clang-format >/dev/null
if [ ! $? -eq 0 ]; then
    echo "Error: please install clang-format on your system."
    exit -1
fi
 
if [[ "${task}" == 'check' ]]; then
    check_syntax
else
 if [[ "${task}" == 'fix' ]]; then
    fix_syntax
 else
    echo "Usage: $0 <check|fix> <rootDir>"
    exit 1
 fi
fi

exit 0

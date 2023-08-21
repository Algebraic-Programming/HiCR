#!/usr/bin/env bash
# Script to check or fix Clang format use
# Taken from https://github.com/cselab/korali/blob/master/tools/style/style_cxx.sh (MIT License ETH Zurich)


if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <check|fix>"
    exit 1
fi
task="${1}"; shift
fileDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# check these destinations
dst=(
    "$fileDir/../examples"
    "$fileDir/../include"
    "$fileDir/../frontends"
    "$fileDir/../tests"
    )

function check()
{
  if [ ! $? -eq 0 ]; then 
    echo "Error fixing style."
    exit -1 
  fi
}

function check_syntax()
{
    for d in "${dst[@]}"; do
      python3 $fileDir/../extern/run-clang-format/run-clang-format.py --recursive ${d} --extensions "hpp,cpp" --exclude "*build/*" --exclude "*extern/*"
      if [ ! $? -eq 0 ]; then
        echo "Error: C++ Code formatting in file ${d} is not normalized."
        echo "Solution: Please run '$0 fix' to fix it."
        exit -1
      fi
    done
}

function fix_syntax()
{
    for d in "${dst[@]}"; do
      basePath=`realpath ${d}` 
      src_files=`find ${basePath} \( -name "*.cpp" -o -name "*.hpp" \) -not -path '*/.*' -not -path '*/build/*' -not -path '*/extern/*'`
      
      if [ ! -z "$src_files" ]
      then
        echo $src_files | xargs -n6 -P2 clang-format -style=file -i "$@"
        check
      fi
    done
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
    fix_syntax
fi

exit 0

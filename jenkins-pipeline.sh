#!/usr/bin/env bash

echo '###############################################################################'
echo '                         Copy repository in container'
echo '###############################################################################'
docker start hicr-buildenv
docker cp $WORKSPACE hicr-buildenv:/home/hicr
docker exec -u root hicr-buildenv chown -R hicr /home/hicr/$JOB_BASE_NAME
if [[ $? -ne "0" ]]; then
    docker stop hicr-buildenv
    exit -1;
fi

echo '###############################################################################'
echo '                                 Meson setup'
echo '###############################################################################'
docker exec -u hicr hicr-buildenv bash -c "mkdir -p /home/hicr/$JOB_BASE_NAME/build && cd /home/hicr/$JOB_BASE_NAME/build && meson setup .. --reconfigure -DbuildExamples=true -DbuildTests=true -Dbackends=hwloc,pthreads,mpi -DHiCR:compileWarningsAsErrors=true" 
if [[ $? -ne "0" ]]; then
    docker stop hicr-buildenv
    exit -1;
fi


echo '###############################################################################'
echo '                                  Compiling'
echo '###############################################################################'
docker exec -u hicr hicr-buildenv bash -c "cd /home/hicr/$JOB_BASE_NAME && meson compile -C build"
if [[ $? -ne "0" ]]; then
    docker stop hicr-buildenv
    exit -1;
fi


echo '###############################################################################'
echo '                                  Run tests'
echo '###############################################################################'
docker exec -u hicr hicr-buildenv bash -c "cd /home/hicr/$JOB_BASE_NAME && meson test -C build"
if [[ $? -ne "0" ]]; then
    docker stop hicr-buildenv
    exit -1;
fi
docker stop hicr-buildenv
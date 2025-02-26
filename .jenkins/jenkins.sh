#!/usr/bin/env bash

export CI_CONTAINER=hicr-ci

echo '###############################################################################'
echo '                         Copy repository in container'
echo '###############################################################################'
docker start hicr-ci
docker cp $WORKSPACE hicr-ci:/home/hicr
docker exec -u root hicr-ci chown -R hicr /home/hicr/$JOB_BASE_NAME
if [[ $? -ne "0" ]]; then
    docker stop hicr-ci
    exit -1;
fi

echo '###############################################################################'
echo '                                 Meson setup'
echo '###############################################################################'
docker exec -u hicr hicr-ci bash -c "mkdir -p /home/hicr/$JOB_BASE_NAME/build && cd /home/hicr/$JOB_BASE_NAME/build && meson setup .. --reconfigure -DbuildExamples=true -DbuildTests=true -Dbackends=hwloc,pthreads,mpi,boost -Dfrontends=tasking,machineModel,deployer,channel -DHiCR:compileWarningsAsErrors=true" 
if [[ $? -ne "0" ]]; then
    docker stop hicr-ci
    exit -1;
fi


echo '###############################################################################'
echo '                                  Compiling'
echo '###############################################################################'
docker exec -u hicr hicr-ci bash -c "cd /home/hicr/$JOB_BASE_NAME && meson compile -C build"
if [[ $? -ne "0" ]]; then
    docker stop hicr-ci
    exit -1;
fi


echo '###############################################################################'
echo '                                  Run tests'
echo '###############################################################################'
docker exec -u hicr hicr-ci bash -c "cd /home/hicr/$JOB_BASE_NAME && meson test -C build"
if [[ $? -ne "0" ]]; then
    docker stop hicr-ci
    exit -1;
fi
docker stop hicr-ci

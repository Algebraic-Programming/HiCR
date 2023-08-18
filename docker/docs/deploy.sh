TAG=`cat tag`

docker login registry.gitlab.huaweirc.ch
docker push "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/docs:${TAG}" 
docker push "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/docs:latest" 
docker rmi -f "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/docs:${TAG}"

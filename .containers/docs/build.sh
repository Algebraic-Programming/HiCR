TAG=`cat tag`

docker build \
	-t "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/docs:${TAG}" \
	-t "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/docs:latest" \
	.

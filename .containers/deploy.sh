if [[ $# -ne 1 ]]; then
   echo "Usage: $0 <name>"
   exit 1
fi
folder=${1}

docker login registry.gitlab.huaweirc.ch
docker push "registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/hicr/${folder}:latest" 

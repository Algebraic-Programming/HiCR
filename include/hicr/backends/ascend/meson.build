# HiCR Ascend Backend Configuration


aclIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include'
])

aclDep = [ ]
    

aclLinkArgs = [
#   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-lascendcl',
#   '-L/usr/local/Ascend/ascend-toolkit/latest/fwkacllib/lib64',
   '-lhccl',
]

aclDeps = declare_dependency(
    dependencies: aclDep,
    link_args : aclLinkArgs,
    include_directories : aclIncludes
)    

hicrAscendBackendDep = [ aclDeps ]
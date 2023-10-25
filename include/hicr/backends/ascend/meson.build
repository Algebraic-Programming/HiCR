# HiCR Ascend Backend Configuration


aclIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include'
])

aclDep = [ ]
    

aclLinkArgs = [
   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-lascendcl',
   '-L/usr/local/Ascend/ascend-toolkit/latest/fwkacllib/lib64',
   '-lhccl',
   '-Wl,-rpath=/usr/local/Ascend/ascend-toolkit/latest/aarch64-linux/devlib',
   '-Wl,-rpath=/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-Wl,-rpath=/usr/local/Ascend/ascend-toolkit/latest/fwkacllib/lib64',
]

aclDeps = declare_dependency(
    dependencies: aclDep,
    link_args : aclLinkArgs,
    include_directories : aclIncludes
)    

hicrAscendBackendDep = [ aclDeps ]

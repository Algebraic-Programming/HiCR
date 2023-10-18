# HiCR Ascend Backend Configuration


aclIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include'
])

aclLinkArgs = [
   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-lascendcl',
]

aclDep = declare_dependency(
    # dependencies: aclDep,
    link_args : aclLinkArgs,
    include_directories : aclIncludes
)    

hicrAscendBackendDep = [ aclDep ]
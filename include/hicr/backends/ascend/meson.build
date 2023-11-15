# HiCR Ascend Backend Configuration


aclIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include'
])

aclDep = [     dependency('threads', required: true),]
    

aclLinkArgs = [
   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-lascendcl',
]

aclDeps = declare_dependency(
    dependencies: aclDep,
    link_args : aclLinkArgs,
    include_directories : aclIncludes
)    

hicrAscendBackendDep = [ aclDeps ]

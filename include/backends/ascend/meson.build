# HiCR Ascend Backend Configuration

backendIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include'
])

backendDependencies = [ ]
    
backendCppArgs = [
   '-D_HICR_USE_ASCEND_BACKEND_'
 ]

backendLinkArgs = [
   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-lascendcl',
]

backendDep = declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

hicrAscendBackendDep = [ backendDep ]

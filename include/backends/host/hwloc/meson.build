# HiCR Hwloc-based Backend Configuration

backendIncludes = include_directories([
])

backendDependencies = [
   dependency('hwloc', required: true)
]
    
backendCppArgs = [
   '-D_HICR_USE_HWLOC_BACKEND_'
 ]

backendLinkArgs = [
]

backendDep = declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

hicrHWLocBackendDep = [ backendDep ]

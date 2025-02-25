# HiCR nOS-V Backend Configuration

nosvDep = dependency('nos-v', required: true, version: '3.1.0')

backendIncludes = include_directories([
])

backendDependencies = [
    nosvDep
]
    
backendCppArgs = [
   '-D_HICR_USE_NOSV_BACKEND_'
 ]

backendLinkArgs = [
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

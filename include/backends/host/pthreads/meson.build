# HiCR Pthreads-based Backend Configuration

backendIncludes = include_directories([
])

backendDependencies = [
    dependency('threads', required: true),
    dependency('boost',  modules : ['context'],  required : true)
]
    
backendCppArgs = [
   '-D_HICR_USE_PTHREADS_BACKEND_'
 ]

backendLinkArgs = [
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

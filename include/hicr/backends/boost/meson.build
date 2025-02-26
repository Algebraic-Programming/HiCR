# HiCR Boost-based Backend Configuration

backendIncludes = include_directories([
])

backendDependencies = [
    dependency('boost', version: '>=1.71', modules : ['context'],  required : true)
]
    
backendCppArgs = [
   '-D_HICR_USE_BOOST_BACKEND_'
 ]

backendLinkArgs = [
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

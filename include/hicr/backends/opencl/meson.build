# HiCR OpenCL Backend Configuration


OpenCLDep = dependency('OpenCL', required: true, version: '3.0')
OpenCLHPPDep = dependency('OpenCL-CLHPP', required: true, version: '3.0')

backendIncludes = include_directories([
])

backendDependencies = [
    OpenCLDep,
    OpenCLHPPDep
]
    
backendCppArgs = [
   '-D_HICR_USE_OPENCL_BACKEND_',
   '-DCL_HPP_TARGET_OPENCL_VERSION=300',
   '-DCL_TARGET_OPENCL_VERSION=300'
 ]

backendLinkArgs = [
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

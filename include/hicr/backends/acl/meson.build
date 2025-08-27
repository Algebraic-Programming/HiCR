# HiCR acl Backend Configuration

backendIncludes = include_directories([
   '/usr/local/Ascend/ascend-toolkit/latest/include'
])

backendDependencies = [ ]
    
backendCppArgs = [
   '-D_HICR_USE_ACL_BACKEND_'
 ]

backendLinkArgs = [
   # TODO: put as meson argument since acl does not provide pkg config or cmake.
   # in the majority of cases this will work since the default installation path is this one
   '-L/usr/local/Ascend/ascend-toolkit/latest/acllib/lib64/stub',
   '-lascendcl',
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

# HiCR Tasking frontend Configuration

frontendIncludes = include_directories([
])

frontendDependencies = [
    dependency('threads', required: true),
]
    
frontendCppArgs = [
   '-D_HICR_USE_TASKING_FRONTEND_'
 ]

frontendLinkArgs = [
]

frontendSrcFiles = [
    'tasking.cpp'
]

HiCRFrontendDependencies += declare_dependency(
    dependencies: frontendDependencies,
    compile_args : frontendCppArgs,
    link_args : frontendLinkArgs,
    include_directories : frontendIncludes,
    sources: frontendSrcFiles
)    

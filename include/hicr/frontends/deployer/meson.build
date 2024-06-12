# HiCR Deployer frontend Configuration

backendIncludes = include_directories([
])

frontendDependencies = [
]
    
frontendCppArgs = [
   '-D_HICR_USE_DEPLOYER_FRONTEND_'
 ]

frontendLinkArgs = [
]

frontendSrcFiles = [
]

HiCRFrontendDependencies += declare_dependency(
    dependencies: frontendDependencies,
    compile_args : frontendCppArgs,
    link_args : frontendLinkArgs,
    include_directories : frontendIncludes,
    sources: frontendSrcFiles
)    

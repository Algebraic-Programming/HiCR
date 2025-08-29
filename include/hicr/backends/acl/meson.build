# HiCR acl Backend Configuration

aclPath = get_option('aclPath')

backendIncludes = []
backendLinkArgs = []
backendDependencies = []
backendCppArgs = ['-D_HICR_USE_ACL_BACKEND_']

if aclPath == ''
   backendDependencies += aclBackendMockSubproject.get_variable('aclDependency')
else
   backendIncludes = include_directories([aclPath + '/include'])
   backendLinkArgs = [
      '-L' + aclPath + '/acllib/lib64/stub',
      '-lascendcl',
   ]
endif

HiCRBackendDependencies += declare_dependency(
   dependencies: backendDependencies,
   compile_args: backendCppArgs,
   link_args: backendLinkArgs,
   include_directories: backendIncludes,
)
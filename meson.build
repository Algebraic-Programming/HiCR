project(
  'HiCR',
  ['cpp', 'c'],
  default_options: [
    'cpp_std=c++20',
    'buildtype=release',
  ],
  subproject_dir: 'extern',
  version: '1.0.0',
)

####### Getting selected built-in backends

# Including selected built-in backends
enabledBackends = get_option('backends')
HiCRBackendDependencies = []


####### Get reference to acl backend mock subproject
if 'acl' in enabledBackends and get_option('aclPath') == ''
   aclBackendMockSubproject = subproject('aclMock', required: true)
endif

foreach backend : enabledBackends
  subdir('include/hicr/backends/' + backend)
endforeach

####### Getting selected built-in frontends

# Including selected built-in frontends
enabledFrontends = get_option('frontends')
HiCRFrontendDependencies = []

foreach frontend : enabledFrontends
  subdir('include/hicr/frontends/' + frontend)
endforeach

####### Creating HiCR Core Configuration and Dependency

subdir('include/hicr/core')

####### Build test / example targets only if HiCR is not being loaded as a subproject

if meson.is_subproject() == false

  # Build example targets
  if get_option('buildExamples')
    subdir('examples')
  endif

  # Build test targets
  if get_option('buildTests')
    subdir('tests')
  endif

endif
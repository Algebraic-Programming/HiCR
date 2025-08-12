# HiCR LPF Backend Configuration

# Try to get MPICH, which is a special case
mpiDep = dependency('mpi', language: 'cpp', required: false)
    
# If also not found, then trigger a failure
if mpiDep.found() == false
  mpiDep = dependency('mpich', required: true)
endif

lpfDep = dependency('lpf-noc', required:false)
# If LPF is not found in the system through e.g. pkg-config, look if libs are present in the environment
if lpfDep.found() == false
  # Now looking for LPF-specific libraries
  cc = meson.get_compiler('cpp')
  lib_lpfcore = cc.find_library('lpf_core_univ_imp_Release') # lpf_core_univ_zero_Debug
  lib_lpfimp = cc.find_library('lpf_core_univ_imp_Release')   # lpf_core_univ_imp_Debug
  lib_lpfhl = cc.find_library('lpf_hl')

  # Update the LPF dependency accordingly
  lpfDep = declare_dependency(
    dependencies: [
      lib_lpfcore,
      lib_lpfhl,
      lib_lpfimp
      ]
    )
endif

lpfrunExecutable = find_program('lpfrun', '/usr/bin/lpfrun', '/usr/local/bin/lpfrun', required : true)

backendDependencies = [
    lpfDep,
    mpiDep
]
    
backendCppArgs = [
   '-D_HICR_USE_LPF_BACKEND_'
 ]

backendLinkArgs = [
]

HiCRBackendDependencies += declare_dependency(
    dependencies: backendDependencies,
    compile_args : backendCppArgs,
    link_args : backendLinkArgs,
    include_directories : backendIncludes
)    

# HiCR LPF Backend Configuration

# Try to get MPICH, which is a special case
mpiDep = dependency('mpi', language: 'cpp', required: true)
    
# If also not found, then trigger a failure
if mpiDep.found() == false
  mpiDep = dependency('mpich', required: false)
endif

# Now looking for LPF-specific libraries
lpfrunExecutable = find_program('lpfrun', '/usr/bin/lpfrun', '/usr/local/bin/lpfrun', required : true)
cc = meson.get_compiler('cpp')
lib_lpfcore = cc.find_library('lpf_core_univ_ibverbs_generic')
lib_lpfimp = cc.find_library('lpf_core_univ_imp_generic')
lib_lpfhl = cc.find_library('lpf_hl')

backendIncludes = include_directories([
])

backendDependencies = [
    lib_lpfcore,
    lib_lpfhl,
    lib_lpfimp,
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

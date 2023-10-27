# HiCR LPF Backend Configuration

# Try to get MPICH, which is a special case
mpiDep = dependency('mpi', language: 'cpp', required: true)
    
# If also not found, then trigger a failure
if mpiDep.found() == false
  mpiDep = dependency('mpich', required: false)
endif

lpfrunExecutable = find_program('lpfrun', '/usr/bin/lpfrun', '/usr/local/bin/lpfrun', required : true)

cc = meson.get_compiler('cpp')
lib_lpfcore = cc.find_library('lpf_core_univ_ibverbs_generic')
lib_lpfhl = cc.find_library('lpf_hl')
hicrLPFBackendDep = declare_dependency( 
dependencies: [lib_lpfcore,lib_lpfhl, mpiDep]
 )    
    



# HiCR LPF Backend Configuration

# Change according to your LPF install dir
lpf_dir = '/home/kdichev/develop-hicr/hicr-0.0.3/subprojects/LPF/build/local'
mpiDep = dependency('mpich', required: true)
cc = meson.get_compiler('cpp')
lib_lpfcore = cc.find_library('lpf_core_univ_ibverbs_generic', dirs: [join_paths(lpf_dir,'lib')])
lib_lpfhl = cc.find_library('lpf_hl', dirs: [join_paths(lpf_dir,'lib')])
inc_lpf = include_directories('../../../../subprojects/LPF/build/local/include')
hicrLPFBackendDep = declare_dependency( 
compile_args: ['-I'+join_paths(lpf_dir,'include')],
dependencies: [lib_lpfcore,lib_lpfhl, mpiDep]
 )    
    



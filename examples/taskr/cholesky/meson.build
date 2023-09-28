choleskyDep = declare_dependency(
    dependencies:  [ dependency('openblas', required: true), dependency('openmp', required: true) ],
    include_directories: include_directories(['source']),
    compile_args: [ '-ffast-math','-Werror' ],
    link_args: [ '-llapacke' ],
    )

# Building raw variant
executable('cholesky_unblocked',
  [ 'source/main.cpp', 'source/variants/unblocked.cpp' ],
  dependencies: choleskyDep,
)

# Building openMP variants
executable('cholesky_blocked_omp_tasks',
  [ 'source/main.cpp', 'source/variants/blocked_omp_tasks.cpp' ],
  dependencies: choleskyDep,
)

executable('cholesky_blocked_omp_threads',
  [ 'source/main.cpp', 'source/variants/blocked_omp_threads.cpp' ],
  dependencies: choleskyDep,
)

# Building TaskR variants
executable('cholesky_blocked_taskr_static',
  [ 'source/main.cpp', 'source/variants/blocked_taskr_static.cpp' ],
  dependencies: [ choleskyDep, taskrBuildDep, hicrSharedMemoryBackendDep ],
)

executable('cholesky_blocked_taskr_dynamic',
  [ 'source/main.cpp', 'source/variants/blocked_taskr_dynamic.cpp' ],
  dependencies: [ choleskyDep, taskrBuildDep, hicrSharedMemoryBackendDep ],
)


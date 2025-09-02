if 'mpi' in enabledBackends and 'hwloc' in enabledBackends
    test(
        'circularBuffer',
        executable(
            'circularBuffer',
            'circularBuffer.cpp',
            dependencies: HiCRTestBuildDependencies,
        ),
        protocol: 'gtest',
        suite: ['tests', 'circular buffer'],
    )
endif
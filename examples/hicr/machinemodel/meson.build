testSuite = [ 'examples', 'hicr', 'machinemodel' ]

incdir = include_directories('source/include')

if 'sharedMemory' in enabledBackends
    machineModel = executable('machineModel', [ 'source/machineModel.cpp'], dependencies: [ hicrBuildDep, hicrSharedMemoryBackendDep], include_directories:incdir)
    test('machineModel', machineModel, args: [ ], suite: testSuite)
endif


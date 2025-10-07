channelsTestIncludeDirs = include_directories(['include'])

channelsTestMPIDep = declare_dependency(
    sources: ['mainMPI.cpp'],
    include_directories: channelsTestIncludeDirs,
    dependencies: [HiCRTestBuildDependencies],
)

subdir('source')
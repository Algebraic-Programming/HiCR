testSuite = [ 'examples', 'hicr', 'getCurrentTask' ]

getCurrentTask = executable('getCurrentTask', [ 'source/main.cpp'], dependencies: [ taskrBuildDep, hicrSequentialBackendDep])
get_option('buildTests') ? test('getCurrentTask', getCurrentTask, args : [ ], suite: testSuite) :


if 'pthreads' in enabledBackends
	groundTruth = executable(
		'groundTruth',
		['main.cpp'],
		include_directories: include_directories(['include']),
		dependencies: [NNHiCRBuildDep],
	)
	test(
		'groundTruth',
		groundTruth,
		args: [
			modelPath,
			testsetDir,
			testsetDir + '/labels.bin',
			'50'
		],
		timeout: 600,
		suite: testSuite,
	)
endif
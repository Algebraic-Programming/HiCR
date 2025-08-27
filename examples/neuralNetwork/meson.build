nnDeps = [hicrBuildDep]

####### Creating Protobuf dependency
####### Defines: onnxProtoDep
subdir('proto')
nnDeps += onnxProtoDep

####### Train network, serialize it in ONNX, and download MNIST test set
####### Defines: modelPath, testsetDir
subdir('python')

####### Compile acl operators
####### Defines: kernelsPath
subdir('kernels')

if 'pthreads' in enabledBackends
  openblasDep = dependency('openblas', required: true)
  nnDeps += openblasDep
endif
  
NNHiCRBuildDep = declare_dependency(
    dependencies: [hicrBuildDep, nnDeps]
)

####### Define test suite
testSuite = ['examples', 'neuralNetwork']

####### Build ground truth executable
subdir('groundTruth')

####### Build neural network that uses HiCR
subdir('source')



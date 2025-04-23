# Train the model if the onnx file is missing
fs = import('fs')

testsetDir = meson.current_source_dir() + '/mnist_images_test'

if not fs.is_dir(testsetDir)
    run_command('download-test-set.sh', venvActivationPath, check: true)
    message('Downloaded MNIST test set')
else
    message('Use existing MNIST test set')
endif

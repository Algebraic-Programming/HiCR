# Train the model if the onnx file is missing
fs = import('fs')

modelPath = meson.current_source_dir() + '/model/neural_network.onnx'

if not fs.is_file(modelPath)
    run_command('train.sh', venvActivationPath, check: true)
    message('Trained network and serialized into neural_network.onnx')
else
    message('Use existing neural_network.onnx')
endif

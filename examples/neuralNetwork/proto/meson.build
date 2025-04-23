####### Creating ONNX Protobuf dependency
protoDep = dependency('protobuf', required: true, version: '29.3.0', modules: ['protobuf::libprotobuf'])

# if the cpp generated protobuf files do not exist run the script to create them
fs = import('fs')
if not fs.is_file(meson.current_source_dir() + '/onnx.proto3.pb.cc') or not fs.is_file(meson.current_source_dir() + '/onnx.proto3.pb.h')
    run_command('compile.sh', check: true)
    message('Compiled onnx.proto3 files')
else
    message('Use existing onnx.proto3.pb files')
endif

onnxProtoDep = declare_dependency(
	# Include the generated onnx protobuf header
	include_directories: include_directories(['.']),
	# Add the generated onnx protobuf source
	sources: ['onnx.proto3.pb.cc'],
	dependencies: [protoDep]
)

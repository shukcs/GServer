package(protobuf)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall")  #c++

INCLUDE_DIRECTORIES(
	.
)
set(SRC_LIST
	google/protobuf/descriptor.cc
	google/protobuf/descriptor.pb.cc
	google/protobuf/descriptor_database.cc
	google/protobuf/dynamic_message.cc
	google/protobuf/extension_set.cc
	google/protobuf/extension_set_heavy.cc
	google/protobuf/generated_message_reflection.cc
	google/protobuf/generated_message_util.cc
	google/protobuf/message.cc
	google/protobuf/message_lite.cc
	google/protobuf/reflection_ops.cc
	google/protobuf/repeated_field.cc
	google/protobuf/service.cc
	google/protobuf/text_format.cc
	google/protobuf/unknown_field_set.cc
	google/protobuf/wire_format.cc
	google/protobuf/wire_format_lite.cc
	google/protobuf/compiler/importer.cc
	google/protobuf/compiler/parser.cc
	google/protobuf/io/coded_stream.cc
	google/protobuf/io/gzip_stream.cc
	google/protobuf/io/printer.cc
	google/protobuf/io/tokenizer.cc
	google/protobuf/io/zero_copy_stream.cc
	google/protobuf/io/zero_copy_stream_impl.cc
	google/protobuf/io/zero_copy_stream_impl_lite.cc
	google/protobuf/stubs/atomicops_internals_x86_gcc.cc
	google/protobuf/stubs/atomicops_internals_x86_msvc.cc
	google/protobuf/stubs/common.cc
	google/protobuf/stubs/once.cc
	google/protobuf/stubs/stringprintf.cc
	google/protobuf/stubs/structurally_valid.cc
	google/protobuf/stubs/strutil.cc
	google/protobuf/stubs/substitute.cc
)

add_library(${PACKAGE_NAME} STATIC ${SRC_LIST})
set_target_properties(
	${PACKAGE_NAME}
	PROPERTIES
	OUTPUT_NAME
	${PACKAGE_NAME}
)

#install(
#	TARGETS
#	${PACKAGE_NAME}
#	#ARCHIVE DESTINATION
#	LIBRARY DESTINATION
#	${PROJECT_BINARY_DIR}
#)

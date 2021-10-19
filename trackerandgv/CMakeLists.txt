package(trackerandgv)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g -rdynamic")  #c++

INCLUDE_DIRECTORIES(
	.
	./src
	./proto
	../protobuf
	../gshare/src
	../gshare/src
	../tinyxml
)
set(SRC_LIST
	../protobuf/ProtoMsg.h
	proto/das.pb.cc
	proto/das.pb.h
	proto/ProtoMsg.cpp
	src/GVManager.cpp
	src/GVManager.h
	src/GXClient.cpp
	src/GXClient.h
	src/GXLink.cpp
	src/GXLink.h
	src/Messages.cpp
	src/Messages.h
	src/ObjectAbsPB.cpp
	src/ObjectAbsPB.h
	src/ObjectGV.cpp
	src/ObjectGV.h
	src/ObjectTracker.cpp
	src/ObjectTracker.h
	src/TrackerManager.cpp
	src/TrackerManager.h
)

add_library(${PACKAGE_NAME} SHARED ${SRC_LIST})
set_target_properties(
	${PACKAGE_NAME}
	PROPERTIES
	OUTPUT_NAME
	${PACKAGE_NAME}
)

target_link_libraries(${PACKAGE_NAME} gshare)
target_link_libraries(${PACKAGE_NAME} tinyxml)
target_link_libraries(${PACKAGE_NAME} protobuf)

#install(
#	TARGETS
#	${PACKAGE_NAME}
#	#ARCHIVE DESTINATION
#	LIBRARY DESTINATION
#	${PROJECT_BINARY_DIR}
#)

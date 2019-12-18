package(uavandqgc)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g")  #c++

INCLUDE_DIRECTORIES(
	.
	./src
	./proto
	../protobuf
	../gshare/src
	../vgmysql/src
	../tinyxml
)
set(SRC_LIST
	proto/das.pb.cc
	proto/das.pb.h
	src/DBManager.cpp
	src/DBManager.h
	src/DBMessages.cpp
	src/DBMessages.h
	src/GSManager.cpp
	src/GSManager.h
	src/FWItem.cpp
	src/FWItem.h
	src/FWAssist.cpp
	src/FWAssist.h
	src/Messages.cpp
	src/Messages.h
	src/ObjectAbsPB.cpp
	src/ObjectAbsPB.h
	src/ObjectGS.cpp
	src/ObjectGS.h
	src/ObjectUav.cpp
	src/ObjectUav.h
	src/ProtoMsg.cpp
	src/ProtoMsg.h
	src/UavManager.cpp
	src/UavManager.h
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
target_link_libraries(${PACKAGE_NAME} vgmysql)

#install(
#	TARGETS
#	${PACKAGE_NAME}
#	#ARCHIVE DESTINATION
#	LIBRARY DESTINATION
#	${PROJECT_BINARY_DIR}
#)

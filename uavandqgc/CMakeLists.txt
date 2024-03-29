package(uavandqgc)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g -rdynamic")  #c++

INCLUDE_DIRECTORIES(
	.
	./src
	./proto
	./mavLink
	../protobuf
	../protobuf/parse/
	../gshare/src
	../vgmysql/src
	../mail/src
	../tinyxml
)
set(SRC_LIST
	proto/das.pb.cc
	proto/das.pb.h
	../protobuf/parse/ProtoMsg.cpp
	../protobuf/parse/ProtoMsg.h
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
	src/UavManager.cpp
	src/UavManager.h
	src/UavMission.cpp
	src/UavMission.h
	src/GXClient.cpp
	src/GXClient.h
	src/GXLink.cpp
	src/GXLink.h
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
target_link_libraries(${PACKAGE_NAME} mail)

#install(
#	TARGETS
#	${PACKAGE_NAME}
#	#ARCHIVE DESTINATION
#	LIBRARY DESTINATION
#	${PROJECT_BINARY_DIR}
#)

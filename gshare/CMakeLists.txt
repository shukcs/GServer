package(gshare)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g -rdynamic")  #c++

add_definitions(-D SHARE_LIBRARY)

INCLUDE_DIRECTORIES(
	.
	./src
	../tinyxml
	../zlib-1.2.11
)
set(SRC_LIST
	src/common/LoopQueue.cpp
	src/common/LoopQueue.h
	src/common/Lock.cpp
	src/common/Lock.h
	src/common/Thread.cpp
	src/common/Thread.h
	src/common/GLoop.cpp
	src/common/GLoop.h
	src/common/Utility.cpp
	src/common/Utility.h
	src/common/Varient.cpp
	src/common/Varient.h
	src/common/Base64.cpp
	src/common/Base64.h
	src/common/MD5lib.cpp
	src/common/MD5lib.h
	src/common/Crc.cpp
	src/common/Crc.h
	src/common/GLibrary.cpp
	src/common/GLibrary.h
	src/GOutLog.cpp
	src/GOutLog.h
	src/gsocket.cpp
	src/gsocket.h
	src/gsocketmanager.cpp
	src/gsocketmanager.h
	src/Ipv4Address.cpp
	src/Ipv4Address.h
	src/Ipv4Address.cpp
	src/Ipv4Address.h
	src/Ipv6Address.cpp
	src/Ipv6Address.h
	src/IPCommon.cpp
	src/IPCommon.h
	src/IMessage.cpp
	src/IMessage.h
	src/ObjectBase.cpp
	src/ObjectBase.h
	src/ObjectManagers.cpp
	src/ObjectManagers.h
	src/BussinessThread.cpp
	src/BussinessThread.h
	src/socket_include.cpp
	src/socket_include.h
)

add_library(${PACKAGE_NAME} STATIC ${SRC_LIST})

target_link_libraries(${PACKAGE_NAME} dl)
target_link_libraries(${PACKAGE_NAME} pthread)
target_link_libraries(${PACKAGE_NAME} zlib)
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

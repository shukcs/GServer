package(gshare)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g")  #c++

add_definitions(-D SHARE_LIBRARY)

INCLUDE_DIRECTORIES(
	.
	./src
	../tinyxml
	../zlib-1.2.11
)
set(SRC_LIST
	src/Base64.cpp
	src/Base64.h
	src/GLibrary.cpp
	src/GLibrary.h
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
	src/Lock.cpp
	src/Lock.h
	src/LoopQueue.cpp
	src/LoopQueue.h
	src/Mutex.cpp
	src/Mutex.h
	src/IMessage.cpp
	src/IMessage.h
	src/ObjectBase.cpp
	src/ObjectBase.h
	src/ObjectManagers.cpp
	src/ObjectManagers.h
	src/socket_include.cpp
	src/socket_include.h
	src/Thread.cpp
	src/Thread.h
	src/Utility.cpp
	src/Utility.h
	src/Varient.cpp
	src/Varient.h
)

add_library(${PACKAGE_NAME} SHARED ${SRC_LIST})

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

package(gshare)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall")  #c++

add_definitions(-D SHARE_LIBRARY)

INCLUDE_DIRECTORIES(
	.
	./src
	../tinyxml
)
set(SRC_LIST
	src/Base64.cpp
	src/Base64.h
	src/BaseBuff.cpp
	src/BaseBuff.h
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
	src/Mutex.cpp
	src/Mutex.h
	src/IMessage.cpp
	src/IMessage.h
	src/ObjectBase.cpp
	src/ObjectBase.h
	src/ObjectManagers.cpp
	src/ObjectManagers.h
	src/Parse.cpp
	src/Parse.h
	src/socket_include.cpp
	src/socket_include.h
	src/Thread.cpp
	src/Thread.h
	src/Utility.cpp
	src/Utility.h
)

add_library(${PACKAGE_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PACKAGE_NAME} dl)
target_link_libraries(${PACKAGE_NAME} pthread)
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

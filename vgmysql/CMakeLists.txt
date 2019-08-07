package(vgmysql)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall")  #c++

add_definitions(-D TIXML_USE_STL)
add_definitions(-D SHARE_LIBRARY)

INCLUDE_DIRECTORIES(
	./src
	../tinyxml
)
set(SRC_LIST
	src/DBExecItem.cpp
	src/DBExecItem.h
	src/VGDBManager.cpp
	src/VGDBManager.h
	src/VGMysql.cpp
	src/VGMysql.h
	src/VGTable.cpp
	src/VGTable.h
)

add_library(${PACKAGE_NAME} SHARED ${SRC_LIST})

target_link_libraries(${PACKAGE_NAME} tinyxml)
target_link_libraries(${PACKAGE_NAME} mysqlclient)
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

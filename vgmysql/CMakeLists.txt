package(vgmysql)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g")  #c++

add_definitions(-D SHARE_LIBRARY)

INCLUDE_DIRECTORIES(
	./src
	../tinyxml
	../gshare/src
)
set(SRC_LIST
	src/DBExecItem.cpp
	src/DBExecItem.h
	src/MysqlDB.cpp
	src/MysqlDB.h
	src/VGMysql.cpp
	src/VGMysql.h
	src/VGTable.cpp
	src/VGTable.h
	src/VGTrigger.cpp
	src/VGTrigger.h
	src/DBManager.cpp
	src/DBManager.h
	src/DBMessages.cpp
	src/DBMessages.h
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
target_link_libraries(${PACKAGE_NAME} mysqlclient)
#install(
#	TARGETS
#	${PACKAGE_NAME}
#	#ARCHIVE DESTINATION
#	LIBRARY DESTINATION
#	${PROJECT_BINARY_DIR}
#)

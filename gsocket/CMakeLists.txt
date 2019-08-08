package(gsocket)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall")  #c++

include_directories(
	.
	../src/
	../gshare/src
)


set(SRC_LIST
	src/main.cpp
)

add_executable(${PACKAGE_NAME} ${SRC_LIST})
set_target_properties(
	${PACKAGE_NAME}
	PROPERTIES
	OUTPUT_NAME
	${PACKAGE_NAME}
)
target_link_libraries(${PACKAGE_NAME} gshare)
target_link_libraries(${PACKAGE_NAME} vgmysql)
#install(
#	TARGETS
#	${PACKAGE_NAME}
#	RUNTIME
#	DESTINATION
#	${PROJECT_BINARY_DIR}
#)

package(gsocket)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g")  #c++

include_directories(
	.
	../src/
	../gshare/src
	../tinyxml
	../mail/src
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

target_link_libraries(${PACKAGE_NAME} tinyxml)
target_link_libraries(${PACKAGE_NAME} gshare)
target_link_libraries(${PACKAGE_NAME} dl)
target_link_libraries(${PACKAGE_NAME} mail)
#install(
#	TARGETS
#	${PACKAGE_NAME}
#	RUNTIME
#	DESTINATION
#	${PROJECT_BINARY_DIR}
#)

package(tinyxml)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall")  #c++

add_definitions(-D _LIB)
add_definitions(-D TIXML_USE_STL)

INCLUDE_DIRECTORIES(
	.
)
set(SRC_LIST
	tinystr.cpp
	tinystr.h
	tinyxml.cpp
	tinyxml.h
	tinyxmlerror.cpp
	tinyxmlparser.cpp
)

add_library(${PACKAGE_NAME} STATIC ${SRC_LIST})
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

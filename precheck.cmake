if(EXISTS ${CMAKE_BINARY_DIR}/CMakeLists.txt)
	message(FATAL_ERROR 
		"In order to prevent commit rubbish into source server, "
		"Remove CMakeCache.txt and CMakeFiles and then use out-of-souce build,Please.\n"
		"See also: http://www.vtk.org/Wiki/CMake_FAQ#Out-of-source_build_trees\n"
		)
endif()

# remove multi-configurations support until cmake has a complete way to support multi-configurations.
# # cmake 2.8 has a bug, when we change CMAKE_CONFIGURATION_TYPES after compiler scan it's no effect so we change it here around this bug.
# if(CMAKE_GENERATOR MATCHES "^Visual Studio")
# 	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configurations")
# endif()
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()
if(CMAKE_GENERATOR MATCHES "^Visual Studio")
	set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "Configurations, do not change it, change CMAKE_BUILD_TYPE please.")
endif()

macro(package _name)
	if(USE_32BITS)
		message(STATUS "using 32bits")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	endif(USE_32BITS)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	set (PACKAGE_NAME ${_name})
	project(${PACKAGE_NAME})
	add_definitions(-std=c++11)
endmacro(package)

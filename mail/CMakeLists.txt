package(mail)
SET(CMAKE_C_FLAGS "-fPIC -O0 -Wall -g")  #c  
SET(CMAKE_CXX_FLAGS "-fPIC -O2 -Wall -g")  #c++

#add_definitions(-D NOCURL)

INCLUDE_DIRECTORIES(
	./src
	../tinyxml
	../gshare/src
)
set(SRC_LIST
	src/MailManager.cpp
	src/MailManager.h
	src/MailMessages.cpp
	src/MailMessages.h
	src/mailSmtp/quickmail.c
	src/mailSmtp/quickmail.h
	src/mailSmtp/smtpsocket.c
	src/mailSmtp/smtpsocket.h
)

add_library(${PACKAGE_NAME} SHARED ${SRC_LIST})

set_target_properties(
	${PACKAGE_NAME}
	PROPERTIES
	OUTPUT_NAME
	${PACKAGE_NAME}
)

target_link_libraries(${PACKAGE_NAME} curl)
target_link_libraries(${PACKAGE_NAME} gshare)
target_link_libraries(${PACKAGE_NAME} tinyxml)

#install(
#	TARGETS
#	${PACKAGE_NAME}
#	#ARCHIVE DESTINATION
#	LIBRARY DESTINATION
#	${PROJECT_BINARY_DIR}
#)

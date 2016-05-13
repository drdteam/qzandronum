project( Masterserver )
cmake_minimum_required( VERSION 2.4 )

include( CheckFunctionExists )

set( ZAN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src )
include_directories( ${ZAN_DIR} )
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

CHECK_FUNCTION_EXISTS( strnicmp STRNICMP_EXISTS )
if( NOT STRNICMP_EXISTS )
   add_definitions( -Dstrnicmp=strncasecmp )
endif( NOT STRNICMP_EXISTS )

add_executable( master-97
	main.cpp
	network.cpp
	${ZAN_DIR}/networkshared.cpp
	${ZAN_DIR}/platform.cpp
	${ZAN_DIR}/huffman/bitreader.cpp 
	${ZAN_DIR}/huffman/bitwriter.cpp 
	${ZAN_DIR}/huffman/huffcodec.cpp 
	${ZAN_DIR}/huffman/huffman.cpp
)

if( WIN32 )
	target_link_libraries( master-97 ws2_32 winmm )
endif( WIN32 )

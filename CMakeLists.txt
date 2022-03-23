## #################################################################
## Root CMakeLists.txt
## #################################################################

cmake_minimum_required(VERSION 3.0)


project(Senet)

if(CMAKE_SYSTEM_NAME MATCHES Windows)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

file( GLOB SENET_SOURCES Senet/*.c Senet/*.cpp)
file( GLOB SENET_HEADERS Senet/*.h )

add_executable( "Senet" ${SENET_SOURCES} ${SENET_HEADERS} )
target_sources("Senet" PRIVATE Senet/Senet.rc)
target_link_libraries("Senet" Comctl32.lib Msimg32.lib)
set_target_properties("Senet" PROPERTIES WIN32_EXECUTABLE TRUE)





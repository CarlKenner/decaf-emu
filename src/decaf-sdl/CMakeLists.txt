project(decaf-sdl)

include_directories(".")

file(GLOB_RECURSE SOURCE_FILES *.cpp)
file(GLOB_RECURSE HEADER_FILES *.h)

if(MSVC)
   set(RESOURCE_FILES ${CMAKE_SOURCE_DIR}/resources/decaf-sdl.rc)
else()
   set(RESOURCE_FILES "")
endif()

include_directories(${CMAKE_SOURCE_DIR}/libraries/LibOVR/include)
			
if(CMAKE_CL_64)
	link_directories(
			${CMAKE_SOURCE_DIR}/libraries/LibOVR/Lib/Windows/x64/Release/VS2015
			${CMAKE_SOURCE_DIR}/libraries/LibOVR/Lib/x64/VS2015)
						
	set(LibOVR_LIBRARIES
		libovr.lib
		ws2_32.lib)
					
else()
	link_directories(
			${CMAKE_SOURCE_DIR}/libraries/LibOVR/Lib/Windows/Win32/Release/VS2015
			${CMAKE_SOURCE_DIR}/libraries/LibOVR/Lib/Win32/VS2015)
					
		set(LibOVR_LIBRARIES
		libovr.lib
		ws2_32.lib)
		
endif()

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		link_directories(${CMAKE_SOURCE_DIR}/libraries/openvr/lib/win32)
	else()
		link_directories(${CMAKE_SOURCE_DIR}/libraries/openvr/lib/win64)
	endif()
endif(WIN32)

if(UNIX)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		link_directories(${CMAKE_SOURCE_DIR}/libraries/openvr/lib/linux32)
	else()
		link_directories(${CMAKE_SOURCE_DIR}/libraries/openvr/lib/linux64)
	endif()
endif(UNIX)

if(APPLE)
	link_directories(${CMAKE_SOURCE_DIR}/libraries/openvr/lib/osx32)
endif(APPLE)

set(OPENVR_LIBRARY "openvr_api")
include_directories("${CMAKE_SOURCE_DIR}/libraries/openvr/headers")

add_executable(decaf-sdl
   ${SOURCE_FILES}
   ${HEADER_FILES}
   ${RESOURCE_FILES})

target_link_libraries(decaf-sdl
    common
    libdecaf
    ${EXCMD_LIBRARIES}
	${LibOVR_LIBRARIES}
	${OPENVR_LIBRARY}
    ${SDL2_LINK})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(decaf-sdl X11)
endif()

if(MSVC)
    set_target_properties(decaf-sdl PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif()

install(TARGETS decaf-sdl RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")

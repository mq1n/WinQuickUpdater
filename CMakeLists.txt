cmake_minimum_required(VERSION 3.2)
project(WinQuickUpdater)

add_definitions(-DFMT_HEADER_ONLY) # fmt

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ../Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ../Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ../Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ../Bin)

if(MSVC)
	set(LIB_RT_SUFFIX "mt")
	set(LIB_RT_OPTION "/MT")

	foreach(flag_var  CMAKE_C_FLAGS  CMAKE_CXX_FLAGS)
		 foreach(config_name  ""  DEBUG  RELEASE  MINSIZEREL  RELWITHDEBINFO)
			set(var_name "${flag_var}")

			if(NOT "${config_name}" STREQUAL "")
				set(var_name "${var_name}_${config_name}")
			endif()
			
			string(REPLACE "/MD" "${LIB_RT_OPTION}" ${var_name} "${${var_name}}")
			set(${var_name} "${${var_name}}" CACHE STRING "" FORCE)
		endforeach()
	endforeach()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNINGS} ${MT_BUILD}")
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	link_directories(${PROJECT_SOURCE_DIR}/extern/lib_x64)
else()
	link_directories(${PROJECT_SOURCE_DIR}/extern/lib_x86)
endif()

include_directories(${PROJECT_SOURCE_DIR}/extern/include)
include_directories(${PROJECT_SOURCE_DIR}/extern/include/imgui)

file(GLOB HEADERS
    "${PROJECT_SOURCE_DIR}/extern/include/imgui/*.h"
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/extern/include/imgui/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(
	${PROJECT_NAME}
	${HEADERS}
	${SOURCES}
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CMAKE_CXX_EXTENSIONS OFF)

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include/pch.hpp)

target_link_libraries(
	${PROJECT_NAME}
	d3d9 d3dx9 DxErr dinput8 dwmapi
)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS    "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

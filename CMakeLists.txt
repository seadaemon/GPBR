cmake_minimum_required (VERSION 3.28)

project(
	GPBR
	VERSION 0.1.0
	LANGUAGES C CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CopyRuntimeDLLs)
include(ShaderHelpers)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Dynamic linking of libraries")

add_subdirectory(gpbr)
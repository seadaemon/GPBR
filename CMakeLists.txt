cmake_minimum_required (VERSION 3.28)

project(
	GPBR
	VERSION 0.1.0
	LANGUAGES C CXX
)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Dynamic linking of libraries")

add_subdirectory(gpbr)
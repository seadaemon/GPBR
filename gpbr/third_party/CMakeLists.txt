# SDL 3
if(NOT BUILD_SHARED_LIBS)
    set(SDL_SHARED_ENABLED_BY_DEFAULT OFF CACHE BOOL "Don't build SDL as a shared library")
endif()
option(SDL_TEST "Build the SDL3_test library" OFF)
add_subdirectory(SDL)

# vk-bootstrap
add_subdirectory(vk-bootstrap)
if (MSVC)
  target_compile_definitions(vk-bootstrap PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()
if (BUILD_SHARED_LIBS)
  set_target_properties(vk-bootstrap PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )
endif()

# fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES
    PUBLIC_HEADER ""
)
if (MSVC)
  target_compile_definitions(fmt PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()
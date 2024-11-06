# SDL 3
if(NOT BUILD_SHARED_LIBS)
    set(SDL_SHARED_ENABLED_BY_DEFAULT OFF CACHE BOOL "Don't build SDL as a shared library")
endif()
option(SDL_TEST "Build the SDL3_test library" OFF)
add_subdirectory(SDL)

# vk-bootstrap
add_subdirectory(vk-bootstrap)
#if (MSVC)
#  target_compile_definitions(vk-bootstrap PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
#endif()
if (BUILD_SHARED_LIBS)
  set_target_properties(vk-bootstrap PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )
endif()

# Dear ImGui
set(IMGUI_SOURCES
    "imgui.cpp"
    "imgui_draw.cpp"
    "imgui_tables.cpp"
    "imgui_widgets.cpp"
    "imgui_demo.cpp"
    # C++ additions
    "misc/cpp/imgui_stdlib.cpp"
    # backend
    "backends/imgui_impl_sdl3.cpp"
    "backends/imgui_impl_vulkan.h"
    "backends/imgui_impl_vulkan.cpp"
)

list(TRANSFORM IMGUI_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/imgui/")

add_library(imgui STATIC ${IMGUI_SOURCES})
set_target_properties(imgui
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK 1)
target_include_directories(imgui
    PUBLIC
        "imgui"
        "imgui/backends"
)

# Fix include issues
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    target_include_directories(imgui PUBLIC ${Vulkan_INCLUDE_DIRS}) 
endif()

if(Vulkan_volk_FOUND)
    target_include_directories(imgui PUBLIC ${Vulkan_INCLUDE_DIRS}/Volk)
endif()


if(BUILD_SHARED_LIBS)
  target_link_libraries(imgui
    PUBLIC
      SDL3::SDL3
  )
else()
  target_link_libraries(imgui
    PUBLIC
      SDL3::SDL3-static
  )
endif()

add_library(imgui::imgui ALIAS imgui)

# fmt
add_subdirectory(fmt)
set_target_properties(fmt PROPERTIES
    PUBLIC_HEADER ""
)
if (MSVC)
  target_compile_definitions(fmt PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)
endif()
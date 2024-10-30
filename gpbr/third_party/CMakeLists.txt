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
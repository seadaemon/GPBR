add_executable(gpbr
	Graphics/Vulkan/vk_types.cpp
    Graphics/Vulkan/vk_engine.cpp
    Graphics/Vulkan/vk_initializers.cpp
    Graphics/Vulkan/vk_images.cpp
)
set_target_properties(gpbr PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)
target_include_directories(gpbr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

target_link_libraries(gpbr PUBLIC
	Vulkan::volk
	vk-bootstrap::vk-bootstrap
	fmt::fmt
)
target_compile_definitions(gpbr
  PUBLIC
    VK_NO_PROTOTYPES
    VMA_VULKAN_VERSION=1003000
)
# SDL
if(BUILD_SHARED_LIBS)
  target_link_libraries(gpbr PUBLIC
    SDL3::SDL3
  )
else()
  target_link_libraries(gpbr PUBLIC
    SDL3::SDL3-static
  )
endif()
# glm
target_compile_definitions(gpbr
  PUBLIC
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_XYZW_ONLY
    GLM_FORCE_EXPLICIT_CTOR
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)
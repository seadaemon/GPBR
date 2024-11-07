add_subdirectory(third_party)

add_executable(gpbr
	src/main.cpp
	src/Graphics/Vulkan/vk_engine.cpp
	src/Graphics/Vulkan/vk_initializers.cpp
	src/Graphics/Vulkan/vk_images.cpp
	src/Graphics/Vulkan/vk_descriptors.cpp
	src/Graphics/Vulkan/vk_pipelines.cpp
)
set_target_properties(gpbr PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
)
if(MSVC)
   set_target_properties(gpbr PROPERTIES DEBUG_POSTFIX _d)
   add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
   add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

target_include_directories(gpbr PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")

# Vulkan - locate the SDK and ensure necessary modules exist
find_package(Vulkan REQUIRED COMPONENTS glslc glslangValidator SPIRV-Tools shaderc_combined volk)

target_link_libraries(gpbr
	PUBLIC
		Vulkan::volk
		vk-bootstrap::vk-bootstrap

		fmt::fmt
		imgui::imgui
)

target_compile_definitions(gpbr
	PUBLIC
		VK_NO_PROTOTYPES
		#VK_USE_64_BIT_PTR_DEFINES=1
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

# copy dlls to build dir
copy_runtime_dlls(gpbr)

# Find shaders in /src/Shaders
FIND_SHADERS()

# Compile shaders and target shaders
TARGET_SHADERS()
add_subdirectory(third_party)

add_executable(gpbr
	src/main.cpp
	src/Graphics/Vulkan/vk_engine.cpp
    src/Graphics/Vulkan/vk_initializers.cpp
	src/Graphics/Vulkan/vk_images.cpp
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

if(Vulkan_FOUND)
	message(STATUS "Vulkan SDK ${Vulkan_VERSION} found!")
else()
	message(FATAL_ERROR "Vulkan SDK not found!")
endif()
if(Vulkan_glslc_FOUND)
	message(STATUS "GLSLC SPIR-V compiler found!")
else()
	message(FATAL_ERROR "GLSLC SPIR-V compiler not found!")
endif()
if(Vulkan_glslangValidator_FOUND)
	message(STATUS "glslangValidator found!")
else()
	message(FATAL_ERROR "glslangValidator not found!")
endif()
if(Vulkan_SPIRV-Tools_FOUND)
	message(STATUS "SPIRV-Tools library found!")
else()
	message(FATAL_ERROR "SPIRV-Tools library not found!")
endif()
if(Vulkan_shaderc_combined_FOUND)
	message(STATUS "shaderc_combined library found!")
else()
	message(FATAL_ERROR "shaderc_combined library not found!")
endif()
if(Vulkan_volk_FOUND)
	message(STATUS "volk library found!")
else()
	message(FATAL_ERROR "volk library not found!")
endif()
message(STATUS "Vulkan_LIBRARY=${Vulkan_LIBRARY}")
message(STATUS "Vulkan_glslang_LIBRARY=${Vulkan_glslang_LIBRARY}")
message(STATUS "Vulkan_SPIRV-Tools_LIBRARY=${Vulkan_SPIRV-Tools_LIBRARY}")
message(STATUS "Vulkan_GLSLANG_VALIDATOR_EXECUTABLE=${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")
# TODO: Find out why FindVulkan.cmake doesnt define Vulkan_glslang_FOUND

target_link_libraries(gpbr
	PUBLIC
		Vulkan::volk
		vk-bootstrap::vk-bootstrap
		fmt::fmt
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

# copy dlls to the same folder as the executable
copy_runtime_dlls(gpbr)
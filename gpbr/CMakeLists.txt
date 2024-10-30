add_subdirectory(third_party)

add_executable(gpbr
	src/main.cpp
)
set_target_properties(gpbr PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
)
if(MSVC)
   set_target_properties(gpbr PROPERTIES DEBUG_POSTFIX _d)
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
# TODO: Find out why FindVulkan.cmake doesnt define the following:
# Vulkan_glslang_FOUND

message(STATUS "Vulkan_LIBRARY=${Vulkan_LIBRARY}")
message(STATUS "Vulkan_glslang_LIBRARY=${Vulkan_glslang_LIBRARY}")
message(STATUS "Vulkan_SPIRV-Tools_LIBRARY=${Vulkan_SPIRV-Tools_LIBRARY}")
message(STATUS "Vulkan_GLSLANG_VALIDATOR_EXECUTABLE=${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")

target_link_libraries(gpbr
	PUBLIC
		Vulkan::volk
		vk-bootstrap::vk-bootstrap
)

target_compile_definitions(gpbr
	PUBLIC
		VK_NO_PROTOTYPES
		VMA_VULKAN_VERSION=1003000
)
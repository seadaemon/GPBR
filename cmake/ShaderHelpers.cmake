# Populates list of shaders ${SHADER_FILES}
macro(FIND_SHADERS)
    set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders")
    message(STATUS "Looking for shaders in ${SHADER_SOURCE_DIR}")
    file(GLOB_RECURSE SHADER_FILES
        "${SHADER_SOURCE_DIR}/*.vert"
        "${SHADER_SOURCE_DIR}/*.frag"
        "${SHADER_SOURCE_DIR}/*.tesc"
        "${SHADER_SOURCE_DIR}/*.tese"
        "${SHADER_SOURCE_DIR}/*.comp"
        "${SHADER_SOURCE_DIR}/*.geom"
    )
endmacro()

macro(TARGET_SHADERS)
    # Create shader folder in build directory
    set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/Shaders)
    file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

    # Build shaders and place them in the build directory
    find_program(GLSLANG_EXECUTABLE glslangValidator
	    HINTS "$ENV{VULKAN_SDK}/bin")
    find_program(SPIRV_OPT_EXECUTABLE spirv-opt
	    HINTS "$ENV{VULKAN_SDK}/bin")

    foreach(SHADER ${SHADER_FILES})
	    get_filename_component(FILENAME ${SHADER} NAME)
	    set(SPV_OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv)
	    add_custom_command(
	    OUTPUT ${SPV_OUTPUT}
	    COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER} -o ${SPV_OUTPUT}
	    DEPENDS ${SHADER}
	    COMMENT "Compiling ${FILENAME}"
	    )
	    list(APPEND SPV_SHADERS ${SPV_OUTPUT})
    endforeach(SHADER)

    add_custom_target(SHADERS ALL DEPENDS ${SPV_SHADERS})

    add_dependencies(gpbr SHADERS)
endmacro()

function(COMPILE_SPIRV_SHADER SHADER_FILE)
    # Define the final name of the generated shader file
    find_program(GLSLANG_EXECUTABLE glslangValidator
        HINTS "$ENV{VULKAN_SDK}/bin")
    find_program(SPIRV_OPT_EXECUTABLE spirv-opt
        HINTS "$ENV{VULKAN_SDK}/bin")
    file(RELATIVE_PATH DEST_SHADER ${CMAKE_SOURCE_DIR} ${SHADER_FILE})
    set(COMPILE_OUTPUT "${CMAKE_BINARY_DIR}/${DEST_SHADER}.debug.spv")
    set(OPTIMIZE_OUTPUT "${CMAKE_BINARY_DIR}/${DEST_SHADER}.spv")
    message(STATUS "Compiling shader ${SHADER_FILE} to ${COMPILE_OUTPUT}")
    add_custom_command(
        OUTPUT ${COMPILE_OUTPUT}
        COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER_FILE} -o ${COMPILE_OUTPUT}
        DEPENDS ${SHADER_FILE})
    add_custom_command(
        OUTPUT ${OPTIMIZE_OUTPUT}
        COMMAND ${SPIRV_OPT_EXECUTABLE} -O ${COMPILE_OUTPUT} -o ${OPTIMIZE_OUTPUT}
        DEPENDS ${COMPILE_OUTPUT})
    set(COMPILE_SPIRV_SHADER_RETURN ${OPTIMIZE_OUTPUT} PARENT_SCOPE)
endfunction()
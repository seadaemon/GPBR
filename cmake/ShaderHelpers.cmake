# NOTE: Changes to shaders are detected, but new shader files require a regeneration

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
	    set(SPV_OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.debug.spv)
        set(OPT_OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv)
	    add_custom_command(
	    OUTPUT ${SPV_OUTPUT}
	    COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER} -o ${SPV_OUTPUT}
	    DEPENDS ${SHADER}
	    COMMENT "Compiling ${FILENAME}"
	    )
        add_custom_command(
	    OUTPUT ${OPT_OUTPUT}
	    COMMAND ${SPIRV_OPT_EXECUTABLE} -O ${SPV_OUTPUT} -o ${OPT_OUTPUT}
	    DEPENDS ${SPV_OUTPUT}
	    #COMMENT "Compiling ${FILENAME}"
	    )
	    list(APPEND SPV_SHADERS ${SPV_OUTPUT})
        list(APPEND OPT_SHADERS ${OPT_OUTPUT})
    endforeach(SHADER)

    add_custom_target(SHADERS ALL DEPENDS ${SPV_SHADERS} ${OPT_SHADERS})

    add_dependencies(gpbr SHADERS)
endmacro()
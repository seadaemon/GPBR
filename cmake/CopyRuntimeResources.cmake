function(copy_runtime_resources target)
	add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime resources to target executable"
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/Shaders"
	  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
	  "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders/"
	  "$<TARGET_FILE_DIR:${target}>/Shaders/"
	  COMMAND_EXPAND_LISTS
	)
endfunction()
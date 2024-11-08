function(copy_assets target)
	add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying assets to target executable folder"
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/Assets"
	  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
	  "${CMAKE_CURRENT_SOURCE_DIR}/assets/"
	  "$<TARGET_FILE_DIR:${target}>/Assets/"
	  COMMAND_EXPAND_LISTS
	)
endfunction()
# TODO: Improve this such that only .spv files are copied over
# Huge thanks to Alex Reinking
# https://stackoverflow.com/questions/14089284/copy-all-dlls-that-an-executable-links-to-to-the-executable-directory/73550650#73550650

function(copy_runtime_dlls target)
	add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E echo "Copying runtime DLLs to target executable"
	  COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:fmt>, $<TARGET_FILE_DIR:${target}>"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
	  "$<TARGET_RUNTIME_DLLS:${target}>"
	  "$<TARGET_FILE_DIR:${target}>"
	  COMMAND_EXPAND_LISTS
	)
endfunction()
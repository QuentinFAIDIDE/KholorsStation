if(NOT DEFINED SOURCE_DIR OR NOT DEFINED PATCH_FILE)
  message(FATAL_ERROR "SOURCE_DIR and PATCH_FILE are required")
endif()

set(patch_files "${PATCH_FILE}")
if(DEFINED SECOND_PATCH_FILE)
  list(APPEND patch_files "${SECOND_PATCH_FILE}")
endif()

foreach(patch_file IN LISTS patch_files)
  execute_process(
    COMMAND git apply --check "${patch_file}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE patch_can_apply)

  if(patch_can_apply EQUAL 0)
    execute_process(
      COMMAND git apply --whitespace=nowarn "${patch_file}"
      WORKING_DIRECTORY "${SOURCE_DIR}"
      COMMAND_ERROR_IS_FATAL ANY)
    continue()
  endif()

  execute_process(
    COMMAND git apply --reverse --check "${patch_file}"
    WORKING_DIRECTORY "${SOURCE_DIR}"
    RESULT_VARIABLE patch_already_applied)

  if(NOT patch_already_applied EQUAL 0)
    message(FATAL_ERROR "Patch cannot be applied: ${patch_file}")
  endif()
endforeach()

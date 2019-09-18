##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# This script, which is run as part of a target, uses pyexpander to do
# macro substitution on an input file and checks the result with the
# version stored in the source code. If the versions are different, an
# error message is printed with further instructions.
#
# To use this script, the CMake variables PYTHON_EXECUTABLE PYEXPANDER_COMMAND,
# SOURCE_FILE, and GENERATED_FILE must be defined as the two files to compare.
# A ".in" is appended to SOURCE_FILE to get the pyexpander input.

if(NOT PYTHON_EXECUTABLE)
  message(SEND_ERROR "Variable PYTHON_EXECUTABLE must be set.")
  return()
endif()

if(NOT PYEXPANDER_COMMAND)
  message(SEND_ERROR "Variable PYEXPANDER_COMMAND must be set.")
  return()
endif()

if(NOT SOURCE_FILE)
  message(SEND_ERROR "Variable SOURCE_FILE must be set.")
  return()
endif()

if(NOT GENERATED_FILE)
  message(SEND_ERROR "Variable GENERATED_FILE must be set.")
  return()
endif()

execute_process(
  COMMAND ${PYTHON_EXECUTABLE} ${PYEXPANDER_COMMAND} ${SOURCE_FILE}.in
  RESULT_VARIABLE pyexpander_result
  OUTPUT_VARIABLE pyexpander_output
  )

if(pyexpander_result)
  # If pyexpander returned non-zero, it failed.
  message(SEND_ERROR "Running pyexpander failed (${pyexpander_result}).")
  return()
endif()

file(WRITE ${GENERATED_FILE}.save "${pyexpander_output}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${SOURCE_FILE} ${GENERATED_FILE}.save
  RESULT_VARIABLE diff_result
  )

if(${diff_result})
  # If diff returned non-zero, it failed and the two files are different.
  get_filename_component(filename ${SOURCE_FILE} NAME)
  message(SEND_ERROR
    "The source file ${filename} does not match the generated file. If you have modified this file directly, then you have messed up. Modify the ${filename}.in file instead and then copy the pyexpander result to ${filename}. If you modified ${filename}.in, then you might just need to copy the pyresult back to the source directory. If you have not modified either, then you have likely checked out an inappropriate change. Check the git logs to see what changes were made.
If the changes have resulted from modifying ${filename}.in, then you can finish by moving ${GENERATED_FILE}.save over ${SOURCE_FILE}")
else()
  # Now that we have done the comparison, remove the generated file so there is
  # no confusion between the generated files and the source files checked into
  # the repository.
  file(REMOVE ${GENERATED_FILE}.save)
  # Pyexpander successfully checked, so touch a file to tell make when the
  # check was last successfully performed.
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_FILE}.checked
    )
endif()

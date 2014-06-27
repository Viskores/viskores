##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 Sandia Corporation.
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014. Los Alamos National Security
##
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

# This script, which is run as part of a target, uses pyexpander to do
# macro substitution on an input file and checks the result with the
# version stored in the source code. If the versions are different, an
# error message is printed with further instructions.
#
# To use this script, the CMake variables PYEXPANDER_COMMAND, SOURCE_FILE,
# and GENERATED_FILE must be defined as the two files to compare. A ".in"
# is appended to SOURCE_FILE to get the pyexpander input.

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
  COMMAND ${PYEXPANDER_COMMAND} ${SOURCE_FILE}.in
  RESULT_VARIABLE pyexpander_result
  OUTPUT_VARIABLE pyexpander_output
  )

if(${pyexpander_result})
  # If pyexpander returned non-zero, it failed.
  message(SEND_ERROR "Running pyexpander failed.")
  return()
endif()

file(WRITE ${GENERATED_FILE} "${pyexpander_output}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${SOURCE_FILE} ${GENERATED_FILE}
  RESULT_VARIABLE diff_result
  )

if(${diff_result})
  # If diff returned non-zero, it failed and the two files are different.
  file(REMOVE ${GENERATED_FILE})
  get_filename_component(filename ${SOURCE_FILE} NAME)
  message(SEND_ERROR
    "The source file ${filename} does not match the generated file. If you have modified this file directly, then you have messed up. Modify the ${filename}.in file instead and then copy the pyexpander result to ${filename}. If you modified ${filename}.in, then you might just need to copy the pyresult back to the source directory. If you have not modifed either, then you have likely checked out an inappropriate change. Check the git logs to see what changes were made.
If the changes have resulted from modifying ${filename}.in, then you can finish by copying ${GENERATED_FILE} to ${SOURCE_FILE}.")
endif()

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

## This CMake script checks source files for the appropriate VTKM copyright
## statement, which is stored in VTKm_SOURCE_DIR/CMake/VTKmCopyrightStatement.txt.
## To run this script, execute CMake as follows:
##
## cmake -DVTKm_SOURCE_DIR=<VTKm_SOURCE_DIR> -P <VTKm_SOURCE_DIR>/CMake/VTKMCheckCopyright.cmake
##

cmake_minimum_required(VERSION 3.8...3.15 FATAL_ERROR)
set(FILES_TO_CHECK
  *.txt
  *.cmake
  *.h
  *.h.in
  *.cxx
  *.cu
  )

set(EXCEPTIONS
  LICENSE.txt
  README.txt
  )

if (NOT VTKm_SOURCE_DIR)
  message(SEND_ERROR "VTKm_SOURCE_DIR not defined.")
endif (NOT VTKm_SOURCE_DIR)

set(copyright_file ${VTKm_SOURCE_DIR}/CMake/VTKmCopyrightStatement.txt)

if (NOT EXISTS ${copyright_file})
  message(SEND_ERROR "Cannot find VTKMCopyrightStatement.txt.")
endif (NOT EXISTS ${copyright_file})

set(license_file ${VTKm_SOURCE_DIR}/LICENSE.txt)

if (NOT EXISTS ${license_file})
  message(SEND_ERROR "Cannot find LICENSE.txt.")
endif (NOT EXISTS ${license_file})

# Get a list of third party files (with different copyrights) from the
# license file.
file(STRINGS ${license_file} license_lines)
list(FIND
  license_lines
  "- - - - - - - - - - - - - - - - - - - - - - - - do not remove this line"
  separator_index
  )
math(EXPR begin_index "${separator_index} + 1")
list(LENGTH license_lines license_file_length)
math(EXPR end_index "${license_file_length} - 1")
foreach (index RANGE ${begin_index} ${end_index})
  list(GET license_lines ${index} tpl_file)
  set(EXCEPTIONS ${EXCEPTIONS} ${tpl_file})
endforeach(index)
# if the build directory is in the source directory, exclude generated build
# files
find_path(BUILD_DIR CMakeCache.txt .)
get_filename_component(abs_build_dir ${BUILD_DIR} ABSOLUTE)
get_filename_component(build_dir_name ${abs_build_dir} NAME)
set(EXCEPTIONS ${EXCEPTIONS} ${build_dir_name}/*)
message("Copyright Check Exceptions: ${EXCEPTIONS}")

# Gets the current year (if possible).
function (get_year var)
  string(TIMESTAMP result "%Y")
  set(${var} "${result}" PARENT_SCOPE)
endfunction (get_year)

set(copyright_file_year 2014)
get_year(current_year)

# Escapes ';' characters (list delimiters) and splits the given string into
# a list of its lines without newlines.
function (list_of_lines var string)
  string(REGEX REPLACE ";" "\\\\;" conditioned_string "${string}")
  string(REGEX REPLACE "\n" ";" conditioned_string "${conditioned_string}")
  set(${var} "${conditioned_string}" PARENT_SCOPE)
endfunction (list_of_lines)

# Read in copyright statement file.
file(READ ${copyright_file} COPYRIGHT_STATEMENT)

# Remove trailing whitespace and ending lines.  They are sometimes hard to
# see or remove in editors.
string(REGEX REPLACE "[ \t]*\n" "\n" COPYRIGHT_STATEMENT "${COPYRIGHT_STATEMENT}")
string(REGEX REPLACE "\n+$" "" COPYRIGHT_STATEMENT "${COPYRIGHT_STATEMENT}")

# Get a list of lines in the copyright statement.
list_of_lines(COPYRIGHT_LINE_LIST "${COPYRIGHT_STATEMENT}")

# Comment regular expression characters that we want to match literally.
string(REPLACE "." "\\." COPYRIGHT_LINE_LIST "${COPYRIGHT_LINE_LIST}")
string(REPLACE "(" "\\(" COPYRIGHT_LINE_LIST "${COPYRIGHT_LINE_LIST}")
string(REPLACE ")" "\\)" COPYRIGHT_LINE_LIST "${COPYRIGHT_LINE_LIST}")

# Introduce regular expression for years we want to be generic.
string(REPLACE
  "${copyright_file_year}"
  "20[0-9][0-9]"
  COPYRIGHT_LINE_LIST
  "${COPYRIGHT_LINE_LIST}"
  )

# Replace year in COPYRIGHT_STATEMENT with current year.
string(REPLACE
  "${copyright_file_year}"
  "${current_year}"
  COPYRIGHT_STATEMENT
  "${COPYRIGHT_STATEMENT}"
  )

# Print an error concerning the missing copyright in the given file.
function(missing_copyright filename comment_prefix)
  message("${filename} does not have the appropriate copyright statement:\n")

  # Condition the copyright statement
  string(REPLACE
    "\n"
    "\n${comment_prefix}  "
    comment_copyright
    "${COPYRIGHT_STATEMENT}"
    )
  set(comment_copyright "${comment_prefix}  ${comment_copyright}")
  string(REPLACE
    "\n${comment_prefix}  \n"
    "\n${comment_prefix}\n"
    comment_copyright
    "${comment_copyright}"
    )

  message("${comment_prefix}=============================================================================")
  message("${comment_prefix}")
  message("${comment_copyright}")
  message("${comment_prefix}")
  message("${comment_prefix}=============================================================================\n")
  message(SEND_ERROR
    "Please add the previous statement to the beginning of ${filename}"
    )
endfunction(missing_copyright)

# Get an appropriate beginning line comment for the given filename.
function(get_comment_prefix var filename)
  get_filename_component(base "${filename}" NAME_WE)
  get_filename_component(extension "${filename}" EXT)
  if (extension STREQUAL ".cmake")
    set(${var} "##" PARENT_SCOPE)
  elseif (base STREQUAL "CMakeLists" AND extension STREQUAL ".txt")
    set(${var} "##" PARENT_SCOPE)
  elseif (extension STREQUAL ".txt")
    set(${var} "" PARENT_SCOPE)
  elseif (extension STREQUAL ".h" OR extension STREQUAL ".h.in" OR extension STREQUAL ".cxx" OR extension STREQUAL ".cu")
    set(${var} "//" PARENT_SCOPE)
  elseif (extension STREQUAL ".worklet")
    set(${var} "//" PARENT_SCOPE)
  else (extension STREQUAL ".cmake")
    message(SEND_ERROR "Could not identify file type of ${filename}.")
  endif (extension STREQUAL ".cmake")
endfunction(get_comment_prefix)

# Check the given file for the appropriate copyright statement.
function(check_copyright filename)

  get_comment_prefix(comment_prefix "${filename}")

  # Read in the first 2000 characters of the file and split into lines.
  # This is roughly equivalent to the file STRINGS command except that we
  # also escape semicolons (list separators) in the input, which the file
  # STRINGS command does not currently do.
  file(READ "${filename}" header_contents LIMIT 2000)
  list_of_lines(header_lines "${header_contents}")

  set(printed)
  # Check each copyright line.
  foreach (copyright_line IN LISTS COPYRIGHT_LINE_LIST)
    set(match)
    # My original algorithm tried to check the order by removing items from
    # header_lines as they were encountered.  Unfortunately, CMake 2.8's
    # list REMOVE_AT command removed the escaping on the ; in one of the
    # header_line's items and cause the compare to fail.
    foreach (header_line IN LISTS header_lines)
      if (copyright_line)
        string(REGEX MATCH
          "^${comment_prefix}[ \t]*${copyright_line}[ \t]*$"
          match
          "${header_line}"
          )
      else (copyright_line)
        if (NOT header_line)
          set(match TRUE)
        endif (NOT header_line)
      endif (copyright_line)
      if (match)
        break()
      endif (match)
    endforeach (header_line)
    if (NOT match AND NOT printed)
      message(STATUS "Could not find match for `${copyright_line}'")
      missing_copyright("${filename}" "${comment_prefix}")
      set(printed TRUE)
    endif (NOT match AND NOT printed)
  endforeach (copyright_line)
endfunction(check_copyright)

foreach (glob_expression ${FILES_TO_CHECK})
  file(GLOB_RECURSE file_list
    RELATIVE "${VTKm_SOURCE_DIR}"
    "${VTKm_SOURCE_DIR}/${glob_expression}"
    )

  foreach (file ${file_list})
    set(skip)
    foreach(exception ${EXCEPTIONS})
      if(file MATCHES "^${exception}(/.*)?$")
        # This file is an exception
        set(skip TRUE)
      endif(file MATCHES "^${exception}(/.*)?$")
    endforeach(exception)

    if (NOT skip)
      check_copyright("${VTKm_SOURCE_DIR}/${file}")
    endif (NOT skip)
  endforeach (file)
endforeach (glob_expression)

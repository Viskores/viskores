##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

## This CMake script checks source files for the appropriate VISKORES license
## statement, which is stored in Viskores_SOURCE_DIR/CMake/ViskoresLicenseStatement.txt.
## To run this script, execute CMake as follows:
##
## cmake -DViskores_SOURCE_DIR=<Viskores_SOURCE_DIR> -P <Viskores_SOURCE_DIR>/CMake/VISKORESCheckLicense.cmake
##

cmake_minimum_required(VERSION 3.15)

set(FILES_TO_CHECK
  *.txt
  *.cmake
  *.h
  *.h.in
  *.hxx
  *.cxx
  *.cu
  *.py
  *.sh
  *.ps1
  Dockerfile
  *.yaml
  *.yml
  )

set(EXCEPTIONS
  LICENSE.txt
  DCO.txt
  README.txt
  docs/users-guide/requirements.txt
  # Common directories with build files
  .venv
  build
  )

if (NOT Viskores_SOURCE_DIR)
  message(SEND_ERROR "Viskores_SOURCE_DIR not defined.")
endif (NOT Viskores_SOURCE_DIR)

set(license_statement_file ${Viskores_SOURCE_DIR}/CMake/ViskoresLicenseStatement.txt)

if (NOT EXISTS ${license_statement_file})
  message(SEND_ERROR "Cannot find VISKORESLicenseStatement.txt.")
endif (NOT EXISTS ${license_statement_file})

set(license_file ${Viskores_SOURCE_DIR}/DCO.txt)

if (NOT EXISTS ${license_file})
  message(SEND_ERROR "Cannot find LICENSE.txt.")
endif (NOT EXISTS ${license_file})

# Get a list of third party files (with different licenses) from the
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
message("License Check Exceptions: ${EXCEPTIONS}")

# Gets the current year (if possible).
function (get_year var)
  string(TIMESTAMP result "%Y")
  set(${var} "${result}" PARENT_SCOPE)
endfunction (get_year)

set(license_file_year 2014)
get_year(current_year)

# Escapes ';' characters (list delimiters) and splits the given string into
# a list of its lines without newlines.
function (list_of_lines var string)
  string(REGEX REPLACE ";" "\\\\;" conditioned_string "${string}")
  string(REGEX REPLACE "\n" ";" conditioned_string "${conditioned_string}")
  set(${var} "${conditioned_string}" PARENT_SCOPE)
endfunction (list_of_lines)

# Read in license statement file.
file(READ ${license_statement_file} LICENSE_STATEMENT)

# Remove trailing whitespace and ending lines.  They are sometimes hard to
# see or remove in editors.
string(REGEX REPLACE "[ \t]*\n" "\n" LICENSE_STATEMENT "${LICENSE_STATEMENT}")
string(REGEX REPLACE "\n+$" "" LICENSE_STATEMENT "${LICENSE_STATEMENT}")

# Get a list of lines in the license statement.
list_of_lines(LICENSE_LINE_LIST "${LICENSE_STATEMENT}")

# Comment regular expression characters that we want to match literally.
string(REPLACE "." "\\." LICENSE_LINE_LIST "${LICENSE_LINE_LIST}")
string(REPLACE "(" "\\(" LICENSE_LINE_LIST "${LICENSE_LINE_LIST}")
string(REPLACE ")" "\\)" LICENSE_LINE_LIST "${LICENSE_LINE_LIST}")

# Introduce regular expression for years we want to be generic.
string(REPLACE
  "${license_file_year}"
  "20[0-9][0-9]"
  LICENSE_LINE_LIST
  "${LICENSE_LINE_LIST}"
  )

# Replace year in LICENSE_STATEMENT with current year.
string(REPLACE
  "${license_file_year}"
  "${current_year}"
  LICENSE_STATEMENT
  "${LICENSE_STATEMENT}"
  )

# Print an error concerning the missing license in the given file.
function(missing_license filename comment_prefix)
  message("${filename} does not have the appropriate license statement:\n")

  # Condition the license statement
  string(REPLACE
    "\n"
    "\n${comment_prefix}  "
    comment_license
    "${LICENSE_STATEMENT}"
    )
  set(comment_license "${comment_prefix}  ${comment_license}")
  string(REPLACE
    "\n${comment_prefix}  \n"
    "\n${comment_prefix}\n"
    comment_license
    "${comment_license}"
    )

  message("${comment_prefix}=============================================================================")
  message("${comment_prefix}")
  message("${comment_license}")
  message("${comment_prefix}")
  message("${comment_prefix}=============================================================================\n")
  message(SEND_ERROR
    "Please add the previous statement to the beginning of ${filename}"
    )
endfunction(missing_license)

# Get an appropriate beginning line comment for the given filename.
function(get_comment_prefix var filename)
  get_filename_component(name "${filename}" NAME)
  if(name MATCHES "\\.(cmake|py|sh|ps1|yaml|yml)$"
    OR name STREQUAL "CMakeLists.txt"
    OR name STREQUAL "Dockerfile"
    )
    set(${var} "##" PARENT_SCOPE)
  elseif (name MATCHES "\\.(h|h\\.in|hxx|cxx|cu)$")
    set(${var} "//" PARENT_SCOPE)
  elseif (name MATCHES "\\.txt$")
    set(${var} "" PARENT_SCOPE)
  else ()
    message(FATAL_ERROR "Could not identify file type of `${name}`.")
  endif ()
endfunction(get_comment_prefix)

# Check the given file for the appropriate license statement.
function(check_license filename)

  get_comment_prefix(comment_prefix "${filename}")

  # Read in the first 2000 characters of the file and split into lines.
  # This is roughly equivalent to the file STRINGS command except that we
  # also escape semicolons (list separators) in the input, which the file
  # STRINGS command does not currently do.
  file(READ "${filename}" header_contents LIMIT 2000)
  list_of_lines(header_lines "${header_contents}")

  set(printed)
  # Check each license line.
  foreach (license_line IN LISTS LICENSE_LINE_LIST)
    set(match)
    # My original algorithm tried to check the order by removing items from
    # header_lines as they were encountered.  Unfortunately, CMake 2.8's
    # list REMOVE_AT command removed the escaping on the ; in one of the
    # header_line's items and cause the compare to fail.
    foreach (header_line IN LISTS header_lines)
      if (license_line)
        string(REGEX MATCH
          "^${comment_prefix}[ \t]*${license_line}[ \t]*$"
          match
          "${header_line}"
          )
      else (license_line)
        if (NOT header_line)
          set(match TRUE)
        endif (NOT header_line)
      endif (license_line)
      if (match)
        break()
      endif (match)
    endforeach (header_line)
    if (NOT match AND NOT printed)
      message(STATUS "Could not find match for `${license_line}'")
      missing_license("${filename}" "${comment_prefix}")
      set(printed TRUE)
    endif (NOT match AND NOT printed)
  endforeach (license_line)
endfunction(check_license)

foreach (glob_expression ${FILES_TO_CHECK})
  file(GLOB_RECURSE file_list
    RELATIVE "${Viskores_SOURCE_DIR}"
    "${Viskores_SOURCE_DIR}/${glob_expression}"
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
      check_license("${Viskores_SOURCE_DIR}/${file}")
    endif (NOT skip)
  endforeach (file)
endforeach (glob_expression)

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# This module is always loaded so that the example code is compiled as part
# of the test build. However, we only really want to build the document if
# Viskores_ENABLE_USERS_GUIDE is set.
if (NOT Viskores_ENABLE_USERS_GUIDE)
  return()
endif()

set(sphinx_docs
  index.rst
  )

find_package(Sphinx)
if (NOT Sphinx_FOUND)
  message(FATAL_ERROR
    "Could not find Sphinx to build User's Guide. If you want to compile Doxygen without the User's Guide, turn off Viskores_ENABLE_USERS_GUIDE.")
endif()

option(Viskores_USERS_GUIDE_INCLUDE_TODOS
  "When building the Viskores User's Guide, specify whether to include TODO notes." ON)
mark_as_advanced(Viskores_USERS_GUIDE_INCLUDE_TODOS)

# Configuration used in conf.py.
set(doxygen_xml_output_dir "${Viskores_BINARY_DIR}/docs/doxygen/xml")
set(example_directory "${CMAKE_CURRENT_SOURCE_DIR}/examples")
if(Viskores_USERS_GUIDE_INCLUDE_TODOS)
  set(include_todos "True")
else()
  set(include_todos "False")
endif()

sphinx_add_docs(ViskoresUsersGuideHTML
  BUILDER html
  SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
  CONF_FILE conf.py
  )
add_dependencies(ViskoresUsersGuideHTML ViskoresDoxygenDocs)

sphinx_add_docs(ViskoresUsersGuideLaTeX
  BUILDER latex
  SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/latex
  CONF_FILE conf.py
  )
add_dependencies(ViskoresUsersGuideLaTeX ViskoresDoxygenDocs)

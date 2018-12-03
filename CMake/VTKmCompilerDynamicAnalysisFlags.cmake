##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
##  Copyright 2014 UT-Battelle, LLC.
##  Copyright 2014 Los Alamos National Security.
##
##  Under the terms of Contract DE-NA0003525 with NTESS,
##  the U.S. Government retains certain rights in this software.
##
##  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
##  Laboratory (LANL), the U.S. Government retains certain rights in
##  this software.
##============================================================================

#-----------------------------------------------------------------------------
# check if this is a sanitizer build. If so, set up the environment.

function(vtkm_check_sanitizer_build)
  string (FIND "${CTEST_MEMORYCHECK_TYPE}" "Sanitizer" SANITIZER_BUILD)
  if (${SANITIZER_BUILD} GREATER -1)
    # This is a sanitizer build.
    # Configure the sanitizer blacklist file
    set (SANITIZER_BLACKLIST "${VTKm_BINARY_DIR}/sanitizer_blacklist.txt")
    configure_file (
      "${VTKm_SOURCE_DIR}/Utilities/DynamicAnalysis/sanitizer_blacklist.txt.in"
      ${SANITIZER_BLACKLIST}
      @ONLY
      )

    # Add the compiler flags for blacklist
    set (FSANITIZE_BLACKLIST "\"-fsanitize-blacklist=${SANITIZER_BLACKLIST}\"")
    foreach (entity C CXX SHARED_LINKER EXE_LINKER MODULE_LINKER)
      set (CMAKE_${entity}_FLAGS "${CMAKE_${entity}_FLAGS} ${FSANITIZE_BLACKLIST}")
    endforeach ()
  endif ()
endfunction()

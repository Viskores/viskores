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
#
# - Finds the pyexpander macro tool.
# Use this module by invoking find_package.
#
# This module finds the expander.py command distributed with pyexpander.
# pyexpander can be downloaded from http://pyexpander.sourceforge.net.
# The following variables are defined:
#
# PYEXPANDER_FOUND   - True if pyexpander is found
# PYEXPANDER_COMMAND - The pyexpander executable
#
# Note that on some platforms (such as Windows), you cannot execute a python
# script directly. Thus, it could be safer to execute the Python interpreter
# with PYEXPANDER_COMMAND as an argument. See FindPythonInterp.cmake for help
# in finding the Python interpreter.
#

find_program(PYEXPANDER_COMMAND expander.py)

mark_as_advanced(PYEXPANDER_COMMAND)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pyexpander DEFAULT_MSG PYEXPANDER_COMMAND)

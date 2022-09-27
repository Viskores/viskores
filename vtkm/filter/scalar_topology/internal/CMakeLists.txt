##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  BranchDecompositionBlock.h
  ComputeBlockIndices.h
  ComputeDistributedBranchDecompositionFunctor.h
  )
#-----------------------------------------------------------------------------

# Note: The C++ source files BranchDecompositionBlock.cxx and
# ComputeDistributedBranchDecompositionFunctor.cxx are added to the scalar
# topology library in the CMakeLists.txt in our parent directory.

vtkm_declare_headers(${headers})

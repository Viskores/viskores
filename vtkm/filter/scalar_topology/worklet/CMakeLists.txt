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
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  )

vtkm_declare_headers(${headers})

add_subdirectory(branch_decomposition)
add_subdirectory(contourtree)
add_subdirectory(contourtree_augmented)
add_subdirectory(contourtree_distributed)

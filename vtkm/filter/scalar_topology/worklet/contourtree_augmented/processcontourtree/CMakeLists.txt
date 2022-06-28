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
  Branch.h
  PiecewiseLinearFunction.h
  SuperArcVolumetricComparator.h
  SuperNodeBranchComparator.h
  PointerDoubling.h
  HypersweepWorklets.h
  SetTriangleSuperarcId.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})

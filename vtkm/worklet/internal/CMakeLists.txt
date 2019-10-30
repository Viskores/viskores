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
  DecayHelpers.h
  DispatcherBase.h
  MaskBase.h
  Placeholders.h
  ScatterBase.h
  TriangulateTables.h
  WorkletBase.h
  )

vtkm_declare_headers(${headers})

add_subdirectory(testing)

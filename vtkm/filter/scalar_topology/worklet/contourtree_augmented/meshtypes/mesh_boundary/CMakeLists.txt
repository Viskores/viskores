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
  MeshBoundary2D.h
  MeshBoundary3D.h
  MeshBoundaryContourTreeMesh.h
  ComputeMeshBoundary2D.h
  ComputeMeshBoundary3D.h
  ComputeMeshBoundaryContourTreeMesh.h
  )

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})

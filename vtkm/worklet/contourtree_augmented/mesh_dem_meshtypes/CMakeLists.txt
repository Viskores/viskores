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
  Freudenthal_2D_Triangulation.h
  Freudenthal_3D_Triangulation.h
  MarchingCubes_3D_Triangulation.h
  ContourTreeMesh.h
  MeshStructureFreudenthal2D.h
  MeshStructureFreudenthal3D.h
  MeshStructureMarchingCubes.h
  MeshStructureContourTreeMesh.h
  MeshBoundary.h
  )

#----------------------------------------------------------------------------
add_subdirectory(contourtreemesh)

#-----------------------------------------------------------------------------
vtkm_declare_headers(${headers})


#-----------------------------------------------------------------------------
set(extra_headers
    freudenthal_2D/Types.h
    freudenthal_3D/Types.h
    marchingcubes_3D/Types.h
    )

#we do the following to work around a couple of issues.
#1. vtkm_declare_headers will do a flat install into the current respective
#   build directory
vtkm_get_kit_name(name dir_prefix)
vtkm_install_headers("${dir_prefix}/freudenthal_2D" freudenthal_2D/Types.h)
vtkm_install_headers("${dir_prefix}/freudenthal_3D" freudenthal_3D/Types.h)
vtkm_install_headers("${dir_prefix}/marchingcubes_3D" marchingcubes_3D/Types.h)

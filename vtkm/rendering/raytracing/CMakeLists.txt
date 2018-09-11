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

set(headers
  BoundingVolumeHierarchy.h
  BVHTraverser.h
  Camera.h
  CellIntersector.h
  CellSampler.h
  CellTables.h
  ChannelBuffer.h
  ChannelBufferOperations.h
  ConnectivityTracer.h
  CylinderExtractor.h
  CylinderIntersector.h
  Logger.h
  MeshConnectivityBuilder.h
  MeshConnectivityContainers.h
  MeshConnectivityBase.h
  MortonCodes.h
  QuadExtractor.h
  QuadIntersector.h
  Ray.h
  RayOperations.h
  RayTracer.h
  RayTracingTypeDefs.h
  Sampler.h
  ShapeIntersector.h
  SphereExtractor.h
  SphereIntersector.h
  TriangleExtractor.h
  TriangleIntersector.h
  VolumeRendererStructured.h
  Worklets.h
  )

set_source_files_properties(CellTables.h
                            PROPERTIES VTKm_CANT_BE_HEADER_TESTED TRUE)

vtkm_declare_headers(${headers})

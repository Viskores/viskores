//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/rendering/raytracing/ConnectivityTracer.h>
#include <vtkm/rendering/raytracing/ConnectivityTracerFactory.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>

#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

//----------------------------------------------------------------------------
ConnectivityTracerFactory::TracerType ConnectivityTracerFactory::DetectCellSetType(
  const vtkm::cont::DynamicCellSet& cellset)
{
  ConnectivityTracerFactory::TracerType type = Unsupported;
  if (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()))
  {
    type = Unstructured;
  }
  else if (cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
  {
    vtkm::cont::CellSetSingleType<> singleType = cellset.Cast<vtkm::cont::CellSetSingleType<>>();
    //
    // Now we need to determine what type of cells this holds
    //
    vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapes =
      singleType.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());
    vtkm::UInt8 shapeType = shapes.GetPortalConstControl().Get(0);
    if (shapeType == CELL_SHAPE_HEXAHEDRON)
      type = UnstructuredHex;
    if (shapeType == CELL_SHAPE_TETRA)
      type = UnstructuredTet;
    if (shapeType == CELL_SHAPE_WEDGE)
      type = UnstructuredWedge;
    if (shapeType == CELL_SHAPE_PYRAMID)
      type = UnstructuredPyramid;
  }
  else if (cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    type = Structured;
  }

  return type;
}


//----------------------------------------------------------------------------
ConnectivityBase* ConnectivityTracerFactory::CreateTracer(
  const vtkm::cont::DynamicCellSet& cellset,
  const vtkm::cont::CoordinateSystem& coords)
{
  ConnectivityTracerFactory::TracerType type = DetectCellSetType(cellset);
  if (type == Unstructured)
  {
    UnstructuredMeshConn meshConn(cellset, coords);
    return new ConnectivityTracer<CELL_SHAPE_ZOO, UnstructuredMeshConn>(meshConn);
  }
  else if (type == UnstructuredHex)
  {
    UnstructuredMeshConnSingleType meshConn(cellset, coords);
    return new ConnectivityTracer<CELL_SHAPE_HEXAHEDRON, UnstructuredMeshConnSingleType>(meshConn);
  }
  else if (type == UnstructuredWedge)
  {
    UnstructuredMeshConnSingleType meshConn(cellset, coords);
    return new ConnectivityTracer<CELL_SHAPE_WEDGE, UnstructuredMeshConnSingleType>(meshConn);
  }
  else if (type == UnstructuredTet)
  {
    UnstructuredMeshConnSingleType meshConn(cellset, coords);

    return new ConnectivityTracer<CELL_SHAPE_TETRA, UnstructuredMeshConnSingleType>(meshConn);
  }
  else if (type == Structured)
  {
    StructuredMeshConn meshConn(cellset, coords);
    return new ConnectivityTracer<CELL_SHAPE_STRUCTURED, StructuredMeshConn>(meshConn);
  }

  throw vtkm::cont::ErrorBadValue("Connectivity tracer: cell set type unsupported");
}
}
}
} // namespace vtkm::rendering::raytracing

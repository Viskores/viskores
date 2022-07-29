//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm
{
namespace cont
{

using UniformType = vtkm::cont::ArrayHandleUniformPointCoordinates;
using Structured2DType = vtkm::cont::CellSetStructured<2>;
using Structured3DType = vtkm::cont::CellSetStructured<3>;

void CellLocatorUniformGrid::Build()
{
  vtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  vtkm::cont::UnknownCellSet cellSet = this->GetCellSet();

  if (!coords.GetData().IsType<UniformType>())
    throw vtkm::cont::ErrorBadType("Coordinates are not uniform type.");

  if (cellSet.CanConvert<Structured2DType>())
  {
    this->Is3D = false;
    Structured2DType structuredCellSet = cellSet.AsCellSet<Structured2DType>();
    vtkm::Id2 pointDims = structuredCellSet.GetSchedulingRange(vtkm::TopologyElementTagPoint());
    this->PointDims = vtkm::Id3(pointDims[0], pointDims[1], 1);
  }
  else if (cellSet.CanConvert<Structured3DType>())
  {
    this->Is3D = true;
    Structured3DType structuredCellSet = cellSet.AsCellSet<Structured3DType>();
    this->PointDims = structuredCellSet.GetSchedulingRange(vtkm::TopologyElementTagPoint());
  }
  else
  {
    throw vtkm::cont::ErrorBadType("Cells are not 2D or 3D structured type.");
  }

  UniformType uniformCoords = coords.GetData().AsArrayHandle<UniformType>();
  auto coordsPortal = uniformCoords.ReadPortal();
  this->Origin = coordsPortal.GetOrigin();

  vtkm::Vec3f spacing = coordsPortal.GetSpacing();
  vtkm::Vec3f unitLength;
  unitLength[0] = static_cast<vtkm::FloatDefault>(this->PointDims[0] - 1);
  unitLength[1] = static_cast<vtkm::FloatDefault>(this->PointDims[1] - 1);
  unitLength[2] = static_cast<vtkm::FloatDefault>(this->PointDims[2] - 1);

  this->MaxPoint = this->Origin + spacing * unitLength;
  this->InvSpacing[0] = 1.f / spacing[0];
  this->InvSpacing[1] = 1.f / spacing[1];
  this->InvSpacing[2] = 1.f / spacing[2];

  this->CellDims[0] = this->PointDims[0] - 1;
  this->CellDims[1] = this->PointDims[1] - 1;
  this->CellDims[2] = this->PointDims[2] - 1;
}

vtkm::exec::CellLocatorUniformGrid CellLocatorUniformGrid::PrepareForExecution(
  vtkm::cont::DeviceAdapterId vtkmNotUsed(device),
  vtkm::cont::Token& vtkmNotUsed(token)) const
{
  this->Update();
  return vtkm::exec::CellLocatorUniformGrid(
    this->CellDims, this->Origin, this->InvSpacing, this->MaxPoint);
}

} //namespace cont
} //namespace vtkm

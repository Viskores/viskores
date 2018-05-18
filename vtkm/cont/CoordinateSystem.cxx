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

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/CoordinateSystem.hxx>

namespace vtkm
{
namespace cont
{

using CoordinatesTypeList = vtkm::ListTagBase<vtkm::cont::ArrayHandleVirtualCoordinates::ValueType>;
using CoordinatesStorageList =
  vtkm::ListTagBase<vtkm::cont::ArrayHandleVirtualCoordinates::StorageTag>;

VTKM_CONT CoordinateSystem::CoordinateSystem()
  : Superclass()
{
}

VTKM_CONT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandleVirtualCoordinates::Superclass& data)
  : Superclass(name, Association::POINTS, data)
{
}

/// This constructor of coordinate system sets up a regular grid of points.
///
VTKM_CONT
CoordinateSystem::CoordinateSystem(std::string name,
                                   vtkm::Id3 dimensions,
                                   vtkm::Vec<vtkm::FloatDefault, 3> origin,
                                   vtkm::Vec<vtkm::FloatDefault, 3> spacing)
  : Superclass(name,
               Association::POINTS,
               vtkm::cont::ArrayHandleVirtualCoordinates(
                 vtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing)))
{
}

VTKM_CONT
vtkm::cont::ArrayHandleVirtualCoordinates CoordinateSystem::GetData() const
{
  return this->Superclass::GetData().Cast<vtkm::cont::ArrayHandleVirtualCoordinates>();
}

VTKM_CONT
void CoordinateSystem::SetData(const vtkm::cont::ArrayHandleVirtualCoordinates::Superclass& newdata)
{
  this->Superclass::SetData(newdata);
}

VTKM_CONT
void CoordinateSystem::PrintSummary(std::ostream& out) const
{
  out << "    Coordinate System ";
  this->Superclass::PrintSummary(out);
}

VTKM_CONT
void CoordinateSystem::GetRange(vtkm::Range* range) const
{
  this->Superclass::GetRange(range, CoordinatesTypeList(), CoordinatesStorageList());
}

VTKM_CONT
const vtkm::cont::ArrayHandle<vtkm::Range>& CoordinateSystem::GetRange() const
{
  return this->Superclass::GetRange(CoordinatesTypeList(), CoordinatesStorageList());
}

VTKM_CONT
vtkm::Bounds CoordinateSystem::GetBounds() const
{
  vtkm::Range ranges[3];
  this->GetRange(ranges);
  return vtkm::Bounds(ranges[0], ranges[1], ranges[2]);
}

template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec<vtkm::FloatDefault, 3>,
    vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec<vtkm::Float32, 3>,
    vtkm::cont::internal::StorageTagCartesianProduct<
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec<vtkm::Float64, 3>,
    vtkm::cont::internal::StorageTagCartesianProduct<
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec<vtkm::Float32, 3>,
    typename vtkm::cont::ArrayHandleCompositeVector<
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>>::StorageTag>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec<vtkm::Float64, 3>,
    typename vtkm::cont::ArrayHandleCompositeVector<
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>>::StorageTag>&);

template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(std::string name,
                                                             const vtkm::cont::DynamicArrayHandle&);

template VTKM_CONT_EXPORT void CoordinateSystem::SetData(
  const vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>>&);
template VTKM_CONT_EXPORT void CoordinateSystem::SetData(
  const vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>>&);
template VTKM_CONT_EXPORT void CoordinateSystem::SetData(const vtkm::cont::DynamicArrayHandle&);
}
} // namespace vtkm::cont

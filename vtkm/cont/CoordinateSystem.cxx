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
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/CoordinateSystem.hxx>

namespace vtkm
{
namespace cont
{

VTKM_CONT CoordinateSystem::CoordinateSystem()
  : Superclass()
{
}

VTKM_CONT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>& data)
  : Superclass(name, Association::POINTS, data)
{
}

/// This constructor of coordinate system sets up a regular grid of points.
///
VTKM_CONT
CoordinateSystem::CoordinateSystem(std::string name,
                                   vtkm::Id3 dimensions,
                                   vtkm::Vec3f origin,
                                   vtkm::Vec3f spacing)
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
void CoordinateSystem::SetData(const vtkm::cont::ArrayHandleVirtual<vtkm::Vec3f>& newdata)
{
  this->Superclass::SetData(newdata);
}

VTKM_CONT
void CoordinateSystem::PrintSummary(std::ostream& out) const
{
  out << "    Coordinate System ";
  this->Superclass::PrintSummary(out);
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
    vtkm::Vec3f,
    vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec3f_32,
    vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec3f_64,
    vtkm::cont::StorageTagCartesianProduct<vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic,
                                           vtkm::cont::StorageTagBasic>>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec3f_32,
    typename vtkm::cont::ArrayHandleCompositeVector<
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float32, vtkm::cont::StorageTagBasic>>::StorageTag>&);
template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(
  std::string name,
  const vtkm::cont::ArrayHandle<
    vtkm::Vec3f_64,
    typename vtkm::cont::ArrayHandleCompositeVector<
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>,
      vtkm::cont::ArrayHandle<vtkm::Float64, vtkm::cont::StorageTagBasic>>::StorageTag>&);

template VTKM_CONT_EXPORT CoordinateSystem::CoordinateSystem(std::string name,
                                                             const vtkm::cont::VariantArrayHandle&);

template VTKM_CONT_EXPORT void CoordinateSystem::SetData(
  const vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>>&);
template VTKM_CONT_EXPORT void CoordinateSystem::SetData(
  const vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>>&);
template VTKM_CONT_EXPORT void CoordinateSystem::SetData(const vtkm::cont::VariantArrayHandle&);
}
} // namespace vtkm::cont

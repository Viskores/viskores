//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CoordianteSystemTransform_h
#define vtk_m_filter_CoordianteSystemTransform_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/CoordinateSystemTransform.h>

namespace vtkm
{
namespace filter
{
/// \brief
///
/// Generate a coordinate transformation on coordiantes from a dataset.
class CylindricalCoordinateTransform
  : public vtkm::filter::FilterField<CylindricalCoordinateTransform>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  CylindricalCoordinateTransform();

  VTKM_CONT void SetCartesianToCylindrical() { Worklet.SetCartesianToCylindrical(); }
  VTKM_CONT void SetCylindricalToCartesian() { Worklet.SetCylindricalToCartesian(); }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  vtkm::worklet::CylindricalCoordinateTransform Worklet;
};

class SphericalCoordinateTransform : public vtkm::filter::FilterField<SphericalCoordinateTransform>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  SphericalCoordinateTransform();

  VTKM_CONT void SetCartesianToSpherical() { Worklet.SetCartesianToSpherical(); }
  VTKM_CONT void SetSphericalToCartesian() { Worklet.SetSphericalToCartesian(); }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<T, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy) const;

private:
  vtkm::worklet::SphericalCoordinateTransform Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/CoordinateSystemTransform.hxx>

#endif // vtk_m_filter_CoordianteSystemTransform_h

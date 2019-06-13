//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PointTransform_h
#define vtk_m_filter_PointTransform_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/PointTransform.h>

namespace vtkm
{
namespace filter
{
/// \brief
///
/// Generate scalar field from a dataset.
template <typename S>
class PointTransform : public vtkm::filter::FilterField<PointTransform<S>>
{
public:
  VTKM_CONT
  PointTransform();

  void SetTranslation(const S& tx, const S& ty, const S& tz);

  void SetTranslation(const vtkm::Vec<S, 3>& v);

  void SetRotation(const S& angleDegrees, const vtkm::Vec<S, 3>& axis);

  void SetRotation(const S& angleDegrees, const S& rx, const S& ry, const S& rz);

  void SetRotationX(const S& angleDegrees);

  void SetRotationY(const S& angleDegrees);

  void SetRotationZ(const S& angleDegrees);

  void SetScale(const S& s);

  void SetScale(const S& sx, const S& sy, const S& sz);

  void SetScale(const vtkm::Vec<S, 3>& v);

  void SetTransform(const vtkm::Matrix<S, 4, 4>& mtx);

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::PointTransform<S> Worklet;
};

template <typename S>
class FilterTraits<PointTransform<S>>
{
public:
  //PointTransformation can only convert Float and Double Vec3 arrays
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/PointTransform.hxx>

#endif // vtk_m_filter_PointTransform_h

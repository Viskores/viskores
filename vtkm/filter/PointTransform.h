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
class PointTransform : public vtkm::filter::FilterField<PointTransform>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  PointTransform();

  void SetTranslation(const vtkm::FloatDefault& tx,
                      const vtkm::FloatDefault& ty,
                      const vtkm::FloatDefault& tz);

  void SetTranslation(const vtkm::Vec3f& v);

  void SetRotation(const vtkm::FloatDefault& angleDegrees, const vtkm::Vec3f& axis);

  void SetRotation(const vtkm::FloatDefault& angleDegrees,
                   const vtkm::FloatDefault& rx,
                   const vtkm::FloatDefault& ry,
                   const vtkm::FloatDefault& rz);

  void SetRotationX(const vtkm::FloatDefault& angleDegrees);

  void SetRotationY(const vtkm::FloatDefault& angleDegrees);

  void SetRotationZ(const vtkm::FloatDefault& angleDegrees);

  void SetScale(const vtkm::FloatDefault& s);

  void SetScale(const vtkm::FloatDefault& sx,
                const vtkm::FloatDefault& sy,
                const vtkm::FloatDefault& sz);

  void SetScale(const vtkm::Vec3f& v);

  void SetTransform(const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& mtx);

  void SetChangeCoordinateSystem(bool flag);
  bool GetChangeCoordinateSystem() const;



  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::PointTransform<vtkm::FloatDefault> Worklet;
  bool ChangeCoordinateSystem;
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_PointTransform_hxx
#include <vtkm/filter/PointTransform.hxx>
#endif

#endif // vtk_m_filter_PointTransform_h

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PointTransform_hxx
#define vtk_m_filter_PointTransform_hxx
#include <vtkm/filter/PointTransform.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT PointTransform::PointTransform()
  : Worklet()
  , ChangeCoordinateSystem(true)
{
  this->SetOutputFieldName("transform");
  this->SetUseCoordinateSystemAsField(true);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetTranslation(const vtkm::FloatDefault& tx,
                                                     const vtkm::FloatDefault& ty,
                                                     const vtkm::FloatDefault& tz)
{
  this->Worklet.SetTranslation(tx, ty, tz);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetTranslation(const vtkm::Vec3f& v)
{
  this->Worklet.SetTranslation(v);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetRotation(const vtkm::FloatDefault& angleDegrees,
                                                  const vtkm::Vec3f& axis)
{
  this->Worklet.SetRotation(angleDegrees, axis);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetRotation(const vtkm::FloatDefault& angleDegrees,
                                                  const vtkm::FloatDefault& rx,
                                                  const vtkm::FloatDefault& ry,
                                                  const vtkm::FloatDefault& rz)
{
  this->Worklet.SetRotation(angleDegrees, rx, ry, rz);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetRotationX(const vtkm::FloatDefault& angleDegrees)
{
  this->Worklet.SetRotationX(angleDegrees);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetRotationY(const vtkm::FloatDefault& angleDegrees)
{
  this->Worklet.SetRotationY(angleDegrees);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetRotationZ(const vtkm::FloatDefault& angleDegrees)
{
  this->Worklet.SetRotationZ(angleDegrees);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetScale(const vtkm::FloatDefault& s)
{
  this->Worklet.SetScale(s);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetScale(const vtkm::FloatDefault& sx,
                                               const vtkm::FloatDefault& sy,
                                               const vtkm::FloatDefault& sz)
{
  this->Worklet.SetScale(sx, sy, sz);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetScale(const vtkm::Vec3f& v)
{
  this->Worklet.SetScale(v);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetTransform(
  const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& mtx)
{
  this->Worklet.SetTransform(mtx);
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void PointTransform::SetChangeCoordinateSystem(bool flag)
{
  this->ChangeCoordinateSystem = flag;
}

//-----------------------------------------------------------------------------
inline VTKM_CONT bool PointTransform::GetChangeCoordinateSystem() const
{
  return this->ChangeCoordinateSystem;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet PointTransform::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<T> outArray;
  this->Invoke(this->Worklet, field, outArray);

  vtkm::cont::DataSet outData =
    CreateResult(inDataSet, outArray, this->GetOutputFieldName(), fieldMetadata);

  if (this->GetChangeCoordinateSystem())
  {
    vtkm::Id coordIndex =
      this->GetUseCoordinateSystemAsField() ? this->GetActiveCoordinateSystemIndex() : 0;
    outData.GetCoordinateSystem(coordIndex).SetData(outArray);
  }

  return outData;
}
}
} // namespace vtkm::filter

#endif //vtk_m_filter_PointTransform_hxx

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT PointTransform<S>::PointTransform()
  : Worklet()
{
  this->SetOutputFieldName("transform");
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetTranslation(const S& tx, const S& ty, const S& tz)
{
  this->Worklet.SetTranslation(tx, ty, tz);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetTranslation(const vtkm::Vec<S, 3>& v)
{
  this->Worklet.SetTranslation(v);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetRotation(const S& angleDegrees,
                                                     const vtkm::Vec<S, 3>& axis)
{
  this->Worklet.SetRotation(angleDegrees, axis);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetRotation(const S& angleDegrees,
                                                     const S& rx,
                                                     const S& ry,
                                                     const S& rz)
{
  this->Worklet.SetRotation(angleDegrees, rx, ry, rz);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetRotationX(const S& angleDegrees)
{
  this->Worklet.SetRotationX(angleDegrees);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetRotationY(const S& angleDegrees)
{
  this->Worklet.SetRotationY(angleDegrees);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetRotationZ(const S& angleDegrees)
{
  this->Worklet.SetRotationZ(angleDegrees);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetScale(const S& s)
{
  this->Worklet.SetScale(s);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetScale(const S& sx, const S& sy, const S& sz)
{
  this->Worklet.SetScale(sx, sy, sz);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetScale(const vtkm::Vec<S, 3>& v)
{
  this->Worklet.SetScale(v);
}

//-----------------------------------------------------------------------------
template <typename S>
inline VTKM_CONT void PointTransform<S>::SetTransform(const vtkm::Matrix<S, 4, 4>& mtx)
{
  this->Worklet.SetTransform(mtx);
}


//-----------------------------------------------------------------------------
template <typename S>
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet PointTransform<S>::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::ArrayHandle<T> outArray;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointTransform<S>> dispatcher(this->Worklet);

  dispatcher.Invoke(field, outArray);

  return internal::CreateResult(inDataSet,
                                outArray,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}
}
} // namespace vtkm::filter

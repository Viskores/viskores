//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleExtrudeField_h
#define vtk_m_cont_ArrayHandleExtrudeField_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageExtrude.h>

namespace vtkm
{
namespace cont
{

template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleExtrudeField
  : public vtkm::cont::ArrayHandle<T, internal::StorageTagExtrude>
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleExtrudeField,
                             (ArrayHandleExtrudeField<T>),
                             (vtkm::cont::ArrayHandle<T, internal::StorageTagExtrude>));

  ArrayHandleExtrudeField(const StorageType& storage)
    : Superclass(storage)
  {
  }

  vtkm::Int32 GetNumberOfValuesPerPlane() const
  {
    return this->GetStorage().GetNumberOfValuesPerPlane();
  }

  vtkm::Int32 GetNumberOfPlanes() const { return this->GetStorage().GetNumberOfPlanes(); }
};

template <typename T>
vtkm::cont::ArrayHandleExtrudeField<T> make_ArrayHandleExtrudeField(
  const T* array,
  vtkm::Id length,
  vtkm::Int32 numberOfPlanes,
  bool cylindrical,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  using StorageType = vtkm::cont::internal::Storage<T, internal::StorageTagExtrude>;
  auto storage =
    StorageType(vtkm::cont::make_ArrayHandle(array, length, copy), numberOfPlanes, cylindrical);
  return ArrayHandleExtrudeField<T>(storage);
}

template <typename T>
vtkm::cont::ArrayHandleExtrudeField<T> make_ArrayHandleExtrudeField(
  const std::vector<T>& array,
  vtkm::Int32 numberOfPlanes,
  bool cylindrical,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandleExtrudeField(
      array.data(), static_cast<vtkm::Id>(array.size()), numberOfPlanes, cylindrical, copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleExtrudeField<T>();
  }
}
}
} // vtkm::cont

#endif

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleExtrudeCoords_h
#define vtk_m_cont_ArrayHandleExtrudeCoords_h

#include <vtkm/cont/StorageExtrude.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/CoordinateSystem.hxx>

namespace vtkm
{
namespace cont
{

template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandleExtrudeCoords
  : public vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, internal::StorageTagExtrude>
{

  using StorageType = vtkm::cont::internal::Storage<vtkm::Vec<T, 3>, internal::StorageTagExtrude>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleExtrudeCoords,
    (ArrayHandleExtrudeCoords<T>),
    (vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, internal::StorageTagExtrude>));

  ArrayHandleExtrudeCoords(const StorageType& storage)
    : Superclass(storage)
  {
  }

  vtkm::Id GetNumberOfPointsPerPlane() const { return (this->GetStorage().GetLength() / 2); }
  vtkm::Int32 GetNumberOfPlanes() const { return this->GetStorage().GetNumberOfPlanes(); }
};

template <typename T>
vtkm::cont::ArrayHandleExtrudeCoords<T> make_ArrayHandleExtrudeCoords(
  const vtkm::cont::ArrayHandle<T> arrHandle,
  vtkm::Int32 numberOfPlanes,
  bool cylindrical)
{
  using StorageType = vtkm::cont::internal::Storage<vtkm::Vec<T, 3>, internal::StorageTagExtrude>;
  auto storage = StorageType(arrHandle, numberOfPlanes, cylindrical);
  return ArrayHandleExtrudeCoords<T>(storage);
}

template <typename T>
vtkm::cont::ArrayHandleExtrudeCoords<T> make_ArrayHandleExtrudeCoords(
  const T* array,
  vtkm::Id length,
  vtkm::Int32 numberOfPlanes,
  bool cylindrical,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  using StorageType = vtkm::cont::internal::Storage<vtkm::Vec<T, 3>, internal::StorageTagExtrude>;
  if (copy == vtkm::CopyFlag::Off)
  {
    return ArrayHandleExtrudeCoords<T>(StorageType(array, length, numberOfPlanes, cylindrical));
  }
  else
  {
    auto storage = StorageType(
      vtkm::cont::make_ArrayHandle(array, length, vtkm::CopyFlag::On), numberOfPlanes, cylindrical);
    return ArrayHandleExtrudeCoords<T>(storage);
  }
}

template <typename T>
vtkm::cont::ArrayHandleExtrudeCoords<T> make_ArrayHandleExtrudeCoords(
  const std::vector<T>& array,
  vtkm::Int32 numberOfPlanes,
  bool cylindrical,
  vtkm::CopyFlag copy = vtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandleExtrudeCoords(
      &array.front(), static_cast<vtkm::Id>(array.size()), numberOfPlanes, cylindrical, copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return ArrayHandleExtrudeCoords<T>();
  }
}
}
}

#endif

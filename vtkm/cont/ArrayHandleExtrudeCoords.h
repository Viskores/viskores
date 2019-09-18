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
  bool GetUseCylindrical() const { return this->GetStorage().GetUseCylindrical(); }
  const vtkm::cont::ArrayHandle<T>& GetArray() const { return this->GetStorage().Array; }
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
} // end namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleExtrudeCoords<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_ExtrudeCoords<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<vtkm::cont::ArrayHandleExtrudeCoords<T>>
{
private:
  using Type = vtkm::cont::ArrayHandleExtrudeCoords<T>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& ah)
  {
    vtkmdiy::save(bb, ah.GetNumberOfPlanes());
    vtkmdiy::save(bb, ah.GetUseCylindrical());
    vtkmdiy::save(bb, ah.GetArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& ah)
  {
    vtkm::Int32 numberOfPlanes;
    bool isCylindrical;
    vtkm::cont::ArrayHandle<T> array;

    vtkmdiy::load(bb, numberOfPlanes);
    vtkmdiy::load(bb, isCylindrical);
    vtkmdiy::load(bb, array);

    ah = vtkm::cont::make_ArrayHandleExtrudeCoords(array, numberOfPlanes, isCylindrical);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif

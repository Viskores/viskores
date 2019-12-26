//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleIndex_h
#define vtk_m_cont_ArrayHandleIndex_h

#include <vtkm/cont/ArrayHandleImplicit.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT StorageTagIndex
{
};

namespace internal
{

struct VTKM_ALWAYS_EXPORT IndexFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id index) const { return index; }
};

using StorageTagIndexSuperclass =
  typename vtkm::cont::ArrayHandleImplicit<IndexFunctor>::StorageTag;

template <>
struct Storage<vtkm::Id, vtkm::cont::StorageTagIndex> : Storage<vtkm::Id, StorageTagIndexSuperclass>
{
  using Superclass = Storage<vtkm::Id, StorageTagIndexSuperclass>;
  using Superclass::Superclass;
};

template <typename Device>
struct ArrayTransfer<vtkm::Id, vtkm::cont::StorageTagIndex, Device>
  : ArrayTransfer<vtkm::Id, StorageTagIndexSuperclass, Device>
{
  using Superclass = ArrayTransfer<vtkm::Id, StorageTagIndexSuperclass, Device>;
  using Superclass::Superclass;
};

} // namespace internal

/// \brief An implicit array handle containing the its own indices.
///
/// \c ArrayHandleIndex is an implicit array handle containing the values
/// 0, 1, 2, 3,... to a specified size. Every value in the array is the same
/// as the index to that value.
///
class ArrayHandleIndex : public vtkm::cont::ArrayHandle<vtkm::Id, StorageTagIndex>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleIndex,
                                (vtkm::cont::ArrayHandle<vtkm::Id, StorageTagIndex>));

  VTKM_CONT
  ArrayHandleIndex(vtkm::Id length)
    : Superclass(typename Superclass::PortalConstControl(internal::IndexFunctor{}, length))
  {
  }
};

/// A convenience function for creating an ArrayHandleIndex. It takes the
/// size of the array and generates an array holding vtkm::Id from [0, size - 1]
VTKM_CONT inline vtkm::cont::ArrayHandleIndex make_ArrayHandleIndex(vtkm::Id length)
{
  return vtkm::cont::ArrayHandleIndex(length);
}
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace vtkm
{
namespace cont
{

template <>
struct SerializableTypeString<vtkm::cont::ArrayHandleIndex>
{
  static VTKM_CONT const std::string Get() { return "AH_Index"; }
};

template <>
struct SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagIndex>>
  : SerializableTypeString<vtkm::cont::ArrayHandleIndex>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<vtkm::cont::ArrayHandleIndex>
{
private:
  using BaseType = vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagIndex>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    vtkmdiy::save(bb, obj.GetNumberOfValues());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    vtkm::Id length = 0;
    vtkmdiy::load(bb, length);

    obj = vtkm::cont::ArrayHandleIndex(length);
  }
};

template <>
struct Serialization<vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagIndex>>
  : Serialization<vtkm::cont::ArrayHandleIndex>
{
};
} // diy
/// @endcond SERIALIZATION

#endif //vtk_m_cont_ArrayHandleIndex_h

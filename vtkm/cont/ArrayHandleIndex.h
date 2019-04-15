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

namespace detail
{

struct VTKM_ALWAYS_EXPORT IndexFunctor
{
  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id index) const { return index; }
};

} // namespace detail

/// \brief An implicit array handle containing the its own indices.
///
/// \c ArrayHandleIndex is an implicit array handle containing the values
/// 0, 1, 2, 3,... to a specified size. Every value in the array is the same
/// as the index to that value.
///
class ArrayHandleIndex : public vtkm::cont::ArrayHandleImplicit<detail::IndexFunctor>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(ArrayHandleIndex,
                                (vtkm::cont::ArrayHandleImplicit<detail::IndexFunctor>));

  VTKM_CONT
  ArrayHandleIndex(vtkm::Id length)
    : Superclass(detail::IndexFunctor(), length)
  {
  }
};
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes

namespace vtkm
{
namespace cont
{

template <>
struct SerializableTypeString<vtkm::cont::detail::IndexFunctor>
{
  static VTKM_CONT const std::string Get() { return "AH_IndexFunctor"; }
};

template <>
struct SerializableTypeString<vtkm::cont::ArrayHandleIndex>
  : SerializableTypeString<vtkm::cont::ArrayHandleImplicit<vtkm::cont::detail::IndexFunctor>>
{
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<vtkm::cont::detail::IndexFunctor>
{
  static VTKM_CONT void save(BinaryBuffer&, const vtkm::cont::detail::IndexFunctor&) {}

  static VTKM_CONT void load(BinaryBuffer&, vtkm::cont::detail::IndexFunctor&) {}
};

template <>
struct Serialization<vtkm::cont::ArrayHandleIndex>
  : Serialization<vtkm::cont::ArrayHandleImplicit<vtkm::cont::detail::IndexFunctor>>
{
};

} // diy

#endif //vtk_m_cont_ArrayHandleIndex_h

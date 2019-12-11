//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_TypeListTag_h
#define vtk_m_TypeListTag_h

// Everything in this header file is deprecated and movded to TypeList.h.

#ifndef VTKM_DEFAULT_TYPE_LIST_TAG
#define VTKM_DEFAULT_TYPE_LIST_TAG ::vtkm::internal::TypeListTagDefault
#endif

#include <vtkm/ListTag.h>
#include <vtkm/TypeList.h>

#define VTK_M_OLD_TYPE_LIST_DEFINITION(name) \
  struct VTKM_ALWAYS_EXPORT \
  VTKM_DEPRECATED( \
    1.6, \
    "TypeListTag" #name " replaced by TypeList" #name ". " \
    "Note that the new TypeList" #name " cannot be subclassed.") \
  TypeListTag ## name : vtkm::internal::ListAsListTag<TypeList ## name> \
  { \
  }

VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{

VTK_M_OLD_TYPE_LIST_DEFINITION(Id);
VTK_M_OLD_TYPE_LIST_DEFINITION(Id2);
VTK_M_OLD_TYPE_LIST_DEFINITION(Id3);
VTK_M_OLD_TYPE_LIST_DEFINITION(IdComponent);
VTK_M_OLD_TYPE_LIST_DEFINITION(Index);
VTK_M_OLD_TYPE_LIST_DEFINITION(FieldScalar);
VTK_M_OLD_TYPE_LIST_DEFINITION(FieldVec2);
VTK_M_OLD_TYPE_LIST_DEFINITION(FieldVec3);
VTK_M_OLD_TYPE_LIST_DEFINITION(FieldVec4);
VTK_M_OLD_TYPE_LIST_DEFINITION(FloatVec);
VTK_M_OLD_TYPE_LIST_DEFINITION(Field);
VTK_M_OLD_TYPE_LIST_DEFINITION(ScalarAll);
VTK_M_OLD_TYPE_LIST_DEFINITION(VecCommon);
VTK_M_OLD_TYPE_LIST_DEFINITION(VecAll);
VTK_M_OLD_TYPE_LIST_DEFINITION(All);
VTK_M_OLD_TYPE_LIST_DEFINITION(Common);

namespace internal
{

VTK_M_OLD_TYPE_LIST_DEFINITION(VecUncommon);

// Special definition of TypeListTagCommon to give descriptive warning when
// VTKM_DEFAULT_TYPE_LIST_TAG is used.
struct VTKM_ALWAYS_EXPORT
VTKM_DEPRECATED(
  1.6,
  "VTKM_DEFAULT_TYPE_LIST_TAG replaced by VTKM_DEFAULT_TYPE_LIST. "
  "Note that the new VTKM_DEFAULT_TYPE_LIST cannot be subclassed.")
TypeListTagDefault : vtkm::internal::ListAsListTag<VTKM_DEFAULT_TYPE_LIST>
{
};

} // namespace internal

// Special implementation of ListContains for TypeListTagAll to always be
// true. Although TypeListTagAll is necessarily finite, the point is to
// be all inclusive. Besides, this should speed up the compilation when
// checking a list that should contain everything.
template<typename Type>
struct ListContains<vtkm::TypeListTagAll, Type>
{
  static constexpr bool value = true;
};

// Special implementation of ListHas for TypeListTagAll to always be
// true. Although TypeListTagAll is necessarily finite, the point is to
// be all inclusive. Besides, this should speed up the compilation when
// checking a list that should contain everything.
namespace detail
{

template<typename Type>
struct ListHasImpl<vtkm::TypeListTagAll, Type>
{
  using type = std::true_type;
};

} // namespace detail

} // namespace vtkm

VTKM_DEPRECATED_SUPPRESS_END
#undef VTK_M_OLD_TYPE_LIST_DEFINITION

#endif //vtk_m_TypeListTag_h

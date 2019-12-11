//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetListTag_h
#define vtk_m_cont_CellSetListTag_h

// Everything in this header file is deprecated and movded to CellSetList.h.

#ifndef VTKM_DEFAULT_CELL_SET_LIST_TAG
#define VTKM_DEFAULT_CELL_SET_LIST_TAG ::vtkm::cont::detail::CellSetListTagDefault
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/CellSetList.h>

#define VTK_M_OLD_CELL_LIST_DEFINITION(name)                                                       \
  struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(                                                       \
    1.6,                                                                                           \
    "CellSetListTag" #name " replaced by CellSetList" #name ". "                                   \
    "Note that the new CellSetList" #name " cannot be subclassed.") CellSetListTag##name           \
    : vtkm::internal::ListAsListTag<CellSetList##name>                                             \
  {                                                                                                \
  }

namespace vtkm
{
namespace cont
{

VTK_M_OLD_CELL_LIST_DEFINITION(Structured1D);
VTK_M_OLD_CELL_LIST_DEFINITION(Structured2D);
VTK_M_OLD_CELL_LIST_DEFINITION(Structured3D);
VTK_M_OLD_CELL_LIST_DEFINITION(ExplicitDefault);
VTK_M_OLD_CELL_LIST_DEFINITION(Common);
VTK_M_OLD_CELL_LIST_DEFINITION(Structured);
VTK_M_OLD_CELL_LIST_DEFINITION(Unstructured);

template <typename ShapesStorageTag = VTKM_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_OFFSETS_STORAGE_TAG>
struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.6,
  "CellSetListTagExplicit replaced by CellSetListExplicit. "
  "Note that the new CellSetListExplicit cannot be subclassed.") CellSetListTagExplicit
  : vtkm::internal::ListAsListTag<
      CellSetListExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>>
{
};

namespace detail
{

struct VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.6,
  "VTKM_DEFAULT_CELL_SET_LIST_TAG replaced by VTKM_DEFAULT_CELL_SET_LIST. "
  "Note that the new VTKM_DEFAULT_CELL_SET_LIST cannot be subclassed.") CellSetListTagDefault
  : vtkm::internal::ListAsListTag<VTKM_DEFAULT_CELL_SET_LIST>
{
};

} // namespace detail
}
} // namespace vtkm::cont

#undef VTK_M_OLD_CELL_LIST_DEFINITION

#endif //vtk_m_cont_CellSetListTag_h

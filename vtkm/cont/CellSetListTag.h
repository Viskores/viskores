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

#ifndef VTKM_DEFAULT_CELL_SET_LIST_TAG
#define VTKM_DEFAULT_CELL_SET_LIST_TAG ::vtkm::cont::CellSetListTagCommon
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm
{
namespace cont
{

struct VTKM_ALWAYS_EXPORT CellSetListTagStructured1D
  : vtkm::ListTagBase<vtkm::cont::CellSetStructured<1>>
{
};

struct VTKM_ALWAYS_EXPORT CellSetListTagStructured2D
  : vtkm::ListTagBase<vtkm::cont::CellSetStructured<2>>
{
};

struct VTKM_ALWAYS_EXPORT CellSetListTagStructured3D
  : vtkm::ListTagBase<vtkm::cont::CellSetStructured<3>>
{
};

template <typename ShapeStorageTag = VTKM_DEFAULT_SHAPE_STORAGE_TAG,
          typename NumIndicesStorageTag = VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_OFFSETS_STORAGE_TAG>
struct VTKM_ALWAYS_EXPORT CellSetListTagExplicit
  : vtkm::ListTagBase<vtkm::cont::CellSetExplicit<ShapeStorageTag,
                                                  NumIndicesStorageTag,
                                                  ConnectivityStorageTag,
                                                  OffsetsStorageTag>>
{
};

struct VTKM_ALWAYS_EXPORT CellSetListTagExplicitDefault : CellSetListTagExplicit<>
{
};

struct VTKM_ALWAYS_EXPORT CellSetListTagCommon : vtkm::ListTagBase<vtkm::cont::CellSetStructured<2>,
                                                                   vtkm::cont::CellSetStructured<3>,
                                                                   vtkm::cont::CellSetExplicit<>,
                                                                   vtkm::cont::CellSetSingleType<>>
{
};

struct VTKM_ALWAYS_EXPORT CellSetListTagStructured
  : vtkm::ListTagBase<vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3>>
{
};

struct VTKM_ALWAYS_EXPORT CellSetListTagUnstructured
  : vtkm::ListTagBase<vtkm::cont::CellSetExplicit<>, vtkm::cont::CellSetSingleType<>>
{
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetListTag_h

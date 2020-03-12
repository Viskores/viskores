//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetList_h
#define vtk_m_cont_CellSetList_h

#ifndef VTKM_DEFAULT_CELL_SET_LIST
#define VTKM_DEFAULT_CELL_SET_LIST ::vtkm::cont::CellSetListCommon
#endif

#include <vtkm/List.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetExtrude.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm
{
namespace cont
{

using CellSetListStructured1D = vtkm::List<vtkm::cont::CellSetStructured<1>>;

using CellSetListStructured2D = vtkm::List<vtkm::cont::CellSetStructured<2>>;

using CellSetListStructured3D = vtkm::List<vtkm::cont::CellSetStructured<3>>;


template <typename ShapesStorageTag = VTKM_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_OFFSETS_STORAGE_TAG>
using CellSetListExplicit = vtkm::List<
  vtkm::cont::CellSetExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>>;

using CellSetListExplicitDefault = CellSetListExplicit<>;

using CellSetListCommon = vtkm::List<vtkm::cont::CellSetStructured<2>,
                                     vtkm::cont::CellSetStructured<3>,
                                     vtkm::cont::CellSetExplicit<>,
                                     vtkm::cont::CellSetSingleType<>>;

using CellSetListStructured =
  vtkm::List<vtkm::cont::CellSetStructured<2>, vtkm::cont::CellSetStructured<3>>;

using CellSetListUnstructured =
  vtkm::List<vtkm::cont::CellSetExplicit<>, vtkm::cont::CellSetSingleType<>>;
}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetList_h

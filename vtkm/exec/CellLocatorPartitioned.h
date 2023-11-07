//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellLocatorPartitioned_h
#define vtk_m_exec_CellLocatorPartitioned_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellLocatorGeneral.h>

namespace vtkm
{
namespace exec
{
class VTKM_ALWAYS_EXPORT CellLocatorPartitioned
{
private:
  vtkm::cont::ArrayHandle<vtkm::cont::CellLocatorGeneral::ExecObjType>::ReadPortalType Locators;
  vtkm::cont::ArrayHandle<
    vtkm::cont::ArrayHandleStride<vtkm::UInt8>::ReadPortalType>::ReadPortalType Ghosts;

public:
  VTKM_CONT CellLocatorPartitioned() = default;
  VTKM_CONT CellLocatorPartitioned(
    const vtkm::cont::ArrayHandle<vtkm::cont::CellLocatorGeneral::ExecObjType>::ReadPortalType&
      locators,
    vtkm::cont::ArrayHandle<
      vtkm::cont::ArrayHandleStride<vtkm::UInt8>::ReadPortalType>::ReadPortalType ghosts)
    : Locators(locators)
    , Ghosts(ghosts)
  {
  }

  VTKM_EXEC
  vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                           vtkm::Id& partitionId,
                           vtkm::Id& cellId,
                           vtkm::Vec3f& parametric) const
  {
    bool found = 0;
    for (vtkm::Id partitionIndex = 0; partitionIndex < this->Locators.GetNumberOfValues();
         ++partitionIndex)
    {
      vtkm::Id cellIndex;
      vtkm ::Vec3f parametricLocal;
      vtkm ::ErrorCode status =
        Locators.Get(partitionIndex).FindCell(point, cellIndex, parametricLocal);
      if (status != vtkm ::ErrorCode ::Success)
      {
      }
      else
      {
        if (Ghosts.Get(partitionIndex).Get(cellIndex) == 0)
        {
          partitionId = partitionIndex;
          cellId = cellIndex;
          parametric = parametricLocal;
          found = true;
          break;
        }
      }
    }
    if (found)
    {
      return vtkm::ErrorCode::Success;
    }
    else
    {
      return vtkm::ErrorCode::CellNotFound;
    }
  }
};
} //namespace exec
} //namespace vtkm

#endif //vtk_m_exec_CellLocatorPartitioned_h

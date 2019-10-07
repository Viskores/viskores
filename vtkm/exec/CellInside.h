//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellInside_h
#define vtk_m_exec_CellInside_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <lcl/lcl.h>

namespace vtkm
{
namespace exec
{

template <typename T, typename CellShapeTag>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, CellShapeTag)
{
  using VtkcTagType = typename vtkm::internal::CellShapeTagVtkmToVtkc<CellShapeTag>::Type;
  return lcl::cellInside(VtkcTagType{}, pcoords);
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>&, vtkm::CellShapeTagEmpty)
{
  return false;
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagPolyLine)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1);
}

/// Checks if the parametric coordinates `pcoords` are on the inside for the
/// specified cell type.
///
template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords,
                                        vtkm::CellShapeTagGeneric shape)
{
  bool result = false;
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(result = CellInside(pcoords, CellShapeTag()));
    default:
      break;
  }

  return result;
}
}
} // vtkm::exec

#endif // vtk_m_exec_CellInside_h

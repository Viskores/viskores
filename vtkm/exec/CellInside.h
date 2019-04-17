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

namespace vtkm
{
namespace exec
{

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>&, vtkm::CellShapeTagEmpty)
{
  return false;
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagVertex)
{
  return pcoords[0] == T(0) && pcoords[1] == T(0) && pcoords[2] == T(0);
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagLine)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1);
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagTriangle)
{
  return pcoords[0] >= T(0) && pcoords[1] >= T(0) && (pcoords[0] + pcoords[1] <= T(1));
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagPolygon)
{
  return ((pcoords[0] - T(0.5)) * (pcoords[0] - T(0.5))) +
    ((pcoords[1] - T(0.5)) * (pcoords[1] - T(0.5))) <=
    T(0.25);
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagQuad)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1) && pcoords[1] >= T(0) && pcoords[1] <= T(1);
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagTetra)
{
  return pcoords[0] >= T(0) && pcoords[1] >= T(0) && pcoords[2] >= T(0) &&
    (pcoords[0] + pcoords[1] + pcoords[2] <= T(1));
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords,
                                        vtkm::CellShapeTagHexahedron)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1) && pcoords[1] >= T(0) && pcoords[1] <= T(1) &&
    pcoords[2] >= T(0) && pcoords[2] <= T(1);
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagWedge)
{
  return pcoords[0] >= T(0) && pcoords[1] >= T(0) && pcoords[2] >= T(0) && pcoords[2] <= T(1) &&
    (pcoords[0] + pcoords[1] <= T(1));
}

template <typename T>
static inline VTKM_EXEC bool CellInside(const vtkm::Vec<T, 3>& pcoords, vtkm::CellShapeTagPyramid)
{
  return pcoords[0] >= T(0) && pcoords[0] <= T(1) && pcoords[1] >= T(0) && pcoords[1] <= T(1) &&
    pcoords[2] >= T(0) && pcoords[2] <= T(1);
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

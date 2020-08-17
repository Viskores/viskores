//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellMeasure_h
#define vtk_m_exec_CellMeasure_h
/*!\file
 * Functions that provide integral measures over cells.
*/

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/ErrorCode.h>
#include <vtkm/VecTraits.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/exec/FunctorBase.h>

namespace vtkm
{
namespace exec
{

/// By default, cells have zero measure unless this template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              CellShapeType shape,
                              vtkm::ErrorCode&)
{
  (void)numPts;
  (void)pts;
  (void)shape;
  return OutType(0.0);
}

// ========================= Arc length cells ==================================
/// Compute the arc length of a poly-line cell.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagLine,
                              vtkm::ErrorCode& ec)
{
  OutType arcLength(0.0);
  if (numPts < 2)
  {
    ec = vtkm::ErrorCode::InvalidCellMetric;
  }
  else
  {
    arcLength = static_cast<OutType>(Magnitude(pts[1] - pts[0]));
    for (int ii = 2; ii < numPts; ++ii)
    {
      arcLength += static_cast<OutType>(Magnitude(pts[ii] - pts[ii - 1]));
    }
  }
  return arcLength;
}

// =============================== Area cells ==================================
/// Compute the area of a triangular cell.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagTriangle,
                              vtkm::ErrorCode& ec)
{
  if (numPts != 3)
  {
    ec = vtkm::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }
  typename PointCoordVecType::ComponentType v1 = pts[1] - pts[0];
  typename PointCoordVecType::ComponentType v2 = pts[2] - pts[0];
  OutType area = OutType(0.5) * static_cast<OutType>(Magnitude(Cross(v1, v2)));
  return area;
}

/// Compute the area of a quadrilateral cell.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagQuad,
                              vtkm::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = vtkm::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  typename PointCoordVecType::ComponentType edges[4] = {
    pts[1] - pts[0],
    pts[2] - pts[1],
    pts[3] - pts[2],
    pts[0] - pts[3],
  };

  typename PointCoordVecType::ComponentType cornerNormals[4] = {
    Cross(edges[3], edges[0]),
    Cross(edges[0], edges[1]),
    Cross(edges[1], edges[2]),
    Cross(edges[2], edges[3]),
  };

  // principal axes
  typename PointCoordVecType::ComponentType principalAxes[2] = {
    edges[0] - edges[2],
    edges[1] - edges[3],
  };

  // Unit normal at the quadrilateral center
  typename PointCoordVecType::ComponentType unitCenterNormal =
    Cross(principalAxes[0], principalAxes[1]);
  Normalize(unitCenterNormal);

  OutType area =
    (Dot(unitCenterNormal, cornerNormals[0]) + Dot(unitCenterNormal, cornerNormals[1]) +
     Dot(unitCenterNormal, cornerNormals[2]) + Dot(unitCenterNormal, cornerNormals[3])) *
    OutType(0.25);
  return area;
}

template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType ComputeMeasure(const vtkm::IdComponent&,
                                 const PointCoordVecType&,
                                 vtkm::CellShapeTagPolygon,
                                 vtkm::ErrorCode& ec)
{
  ec = vtkm::ErrorCode::InvalidCellMetric;
  return OutType(0.0);
}


// ============================= Volume cells ==================================
/// Compute the volume of a tetrahedron.
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagTetra,
                              vtkm::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = vtkm::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  typename PointCoordVecType::ComponentType v1 = pts[1] - pts[0];
  typename PointCoordVecType::ComponentType v2 = pts[2] - pts[0];
  typename PointCoordVecType::ComponentType v3 = pts[3] - pts[0];
  OutType volume = Dot(Cross(v1, v2), v3) / OutType(6.0);
  return volume;
}

/// Compute the volume of a hexahedral cell (approximated via triple product of average edge along each parametric axis).
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagHexahedron,
                              vtkm::ErrorCode& ec)
{
  if (numPts != 8)
  {
    ec = vtkm::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  auto efg1 = pts[1];
  efg1 += pts[2];
  efg1 += pts[5];
  efg1 += pts[6];
  efg1 -= pts[0];
  efg1 -= pts[3];
  efg1 -= pts[4];
  efg1 -= pts[7];

  auto efg2 = pts[2];
  efg2 += pts[3];
  efg2 += pts[6];
  efg2 += pts[7];
  efg2 -= pts[0];
  efg2 -= pts[1];
  efg2 -= pts[4];
  efg2 -= pts[5];

  auto efg3 = pts[4];
  efg3 += pts[5];
  efg3 += pts[6];
  efg3 += pts[7];
  efg3 -= pts[0];
  efg3 -= pts[1];
  efg3 -= pts[2];
  efg3 -= pts[3];

  OutType volume = Dot(Cross(efg2, efg3), efg1) / OutType(64.0);
  return volume;
}

/// Compute the volume of a wedge cell (approximated as 3 tetrahedra).
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagWedge,
                              vtkm::ErrorCode& ec)
{
  if (numPts != 6)
  {
    ec = vtkm::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  typename PointCoordVecType::ComponentType v0 = pts[1] - pts[0];
  typename PointCoordVecType::ComponentType v1 = pts[2] - pts[0];
  typename PointCoordVecType::ComponentType v2 = pts[3] - pts[0];
  OutType volume = Dot(Cross(v0, v1), v2) / OutType(6.0);

  typename PointCoordVecType::ComponentType v3 = pts[4] - pts[1];
  typename PointCoordVecType::ComponentType v4 = pts[5] - pts[1];
  typename PointCoordVecType::ComponentType v5 = pts[3] - pts[1];
  volume += Dot(Cross(v3, v4), v5) / OutType(6.0);

  typename PointCoordVecType::ComponentType v6 = pts[5] - pts[1];
  typename PointCoordVecType::ComponentType v7 = pts[2] - pts[1];
  typename PointCoordVecType::ComponentType v8 = pts[3] - pts[1];
  volume += Dot(Cross(v6, v7), v8) / OutType(6.0);

  return volume;
}

/// Compute the volume of a pyramid (approximated as 2 tetrahedra)
template <typename OutType, typename PointCoordVecType>
VTKM_EXEC OutType CellMeasure(const vtkm::IdComponent& numPts,
                              const PointCoordVecType& pts,
                              vtkm::CellShapeTagPyramid,
                              vtkm::ErrorCode& ec)
{
  if (numPts != 5)
  {
    ec = vtkm::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  typename PointCoordVecType::ComponentType v1 = pts[1] - pts[0];
  typename PointCoordVecType::ComponentType v2 = pts[3] - pts[0];
  typename PointCoordVecType::ComponentType v3 = pts[4] - pts[0];
  OutType volume = Dot(Cross(v1, v2), v3) / OutType(6.0);

  typename PointCoordVecType::ComponentType v4 = pts[1] - pts[2];
  typename PointCoordVecType::ComponentType v5 = pts[3] - pts[2];
  typename PointCoordVecType::ComponentType v6 = pts[4] - pts[2];
  volume += Dot(Cross(v5, v4), v6) / OutType(6.0);

  return volume;
}
}
}

#endif // vtk_m_exec_CellMeasure_h
